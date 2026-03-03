#include "UInput.hpp"

#include <linux/uinput.h>
#include <poll.h>
#include <sys/ioctl.h>

#include <fmt/format.h>

namespace phant {
    UInputController::UInputController() = default;

    UInputController::~UInputController() {
        m_stopFF.store(true, std::memory_order_relaxed);
        if (m_ffThread.joinable()) m_ffThread.join();
        if (m_fd >= 0) {
            ::ioctl(m_fd, UI_DEV_DESTROY);
            ::close(m_fd);
        }
    }

    geode::Result<std::unique_ptr<UInputController>> UInputController::create(
        std::string_view name,
        RumbleCallback rumbleCallback,
        bool enableGyro
    ) {
        int fd = ::open("/dev/uinput", O_RDWR | O_NONBLOCK);
        if (fd < 0) {
            return geode::Err(fmt::format("Failed to open /dev/uinput: {}", ::strerror(errno)));
        }

        auto ioctl = [&]<typename... Args>(int request, Args&&... args) -> geode::Result<> {
            if (::ioctl(fd, request, std::forward<Args>(args)...) < 0) {
                ::close(fd);
                return geode::Err(fmt::format("ioctl failed: {}", ::strerror(errno)));
            }
            return geode::Ok();
        };

        GEODE_UNWRAP(ioctl(UI_SET_EVBIT, EV_KEY));
        GEODE_UNWRAP(ioctl(UI_SET_EVBIT, EV_ABS));
        GEODE_UNWRAP(ioctl(UI_SET_EVBIT, EV_SYN));
        GEODE_UNWRAP(ioctl(UI_SET_EVBIT, EV_FF));
        GEODE_UNWRAP(ioctl(UI_SET_FFBIT, FF_RUMBLE));

        for (auto const& [bit, code] : BUTTON_MAPPING) {
            GEODE_UNWRAP(ioctl(UI_SET_KEYBIT, code));
        }

        size_t axisCount = enableGyro ? AXIS_COUNT_GYRO : AXIS_COUNT_NO_GYRO;

        for (size_t i = 0; i < axisCount; ++i) {
            auto const& setup = AXIS_SETUP[i];
            GEODE_UNWRAP(ioctl(UI_SET_ABSBIT, setup.code));

            uinput_abs_setup s{
                .code = static_cast<uint16_t>(setup.code),
                .absinfo = {
                    .minimum = setup.min,
                    .maximum = setup.max,
                    .fuzz = setup.fuzz,
                    .flat = setup.flat,
                }
            };

            GEODE_UNWRAP(ioctl(UI_ABS_SETUP, &s));
        }

        uinput_setup setup{
            .id = {
                .bustype = BUS_USB,
                .vendor = 0x045e,
                .product = 0x028e,
                .version = 0x0110
            },
            .ff_effects_max = MAX_FF_EFFECTS
        };

        std::memcpy(setup.name, name.data(), std::min<size_t>(name.size(), sizeof(setup.name) - 1));

        GEODE_UNWRAP(ioctl(UI_DEV_SETUP, &setup));
        GEODE_UNWRAP(ioctl(UI_DEV_CREATE));

        auto controller = std::make_unique<UInputController>();
        controller->m_fd = fd;
        controller->m_rumbleCallback = std::move(rumbleCallback);
        controller->m_ffThread = std::thread(&UInputController::workerThread, controller.get());

        return geode::Ok(std::move(controller));
    }

    void UInputController::setState(ControllerState const& state) {
        this->diffButtons(state);
        this->diffAxes(state);
        m_state = state;

        this->sync();
    }

    void UInputController::sync() {
        this->pushEvent(EV_SYN, SYN_REPORT, 0);
        this->flushBuffer();
    }

    void UInputController::waitForDevice(int fd) {
        char sysname[64]{};
        if (::ioctl(fd, UI_GET_SYSNAME(sizeof(sysname)), sysname) < 0) return;

        std::string devPath = fmt::format("/dev/input/{}", sysname);
        for (int i = 0; i < 100; ++i) {
            if (::access(devPath.c_str(), F_OK) == 0) return;
            ::usleep(5'000);
        }
    }

    void UInputController::workerThread() {
        pollfd pfd{m_fd, POLLIN, 0};

        while (!m_stopFF.load(std::memory_order_relaxed)) {
            if (::poll(&pfd, 1, 10) <= 0) continue;

            input_event ev{};
            while (::read(m_fd, &ev, sizeof(ev)) == sizeof(ev)) {
                if (ev.type == EV_UINPUT) this->handleFFControl(ev);
                else if (ev.type == EV_FF) this->handleFFPlay(ev);
            }
        }
    }

    void UInputController::handleFFControl(input_event const& event) {
        if (event.code == UI_FF_UPLOAD) {
            uinput_ff_upload upload{.request_id = static_cast<uint32_t>(event.value)};
            if (::ioctl(m_fd, UI_BEGIN_FF_UPLOAD, &upload) < 0) return;
            upload.retval = 0;
            if (upload.effect.type == FF_RUMBLE) {
                m_ffEffects[upload.effect.id] = upload.effect;
            }
            ::ioctl(m_fd, UI_END_FF_UPLOAD, &upload);
        } else if (event.code == UI_FF_ERASE) {
            uinput_ff_erase erase{.request_id = static_cast<uint32_t>(event.value)};
            if (::ioctl(m_fd, UI_BEGIN_FF_ERASE, &erase) < 0) return;
            erase.retval = 0;
            m_ffEffects.erase(erase.effect_id);
            ::ioctl(m_fd, UI_END_FF_ERASE, &erase);
        }
    }

    void UInputController::handleFFPlay(input_event const& event) {
        auto it = m_ffEffects.find(event.code);
        if (it == m_ffEffects.end()) return;

        auto const& rumble = it->second.u.rumble;
        m_rumbleCallback(
            event.value ? rumble.strong_magnitude : 0,
            event.value ? rumble.weak_magnitude : 0
        );
    }

    void UInputController::pushEvent(uint16_t type, uint16_t code, int32_t value) {
        if (m_eventCount >= EVENT_BUFFER_SIZE) {
            this->flushBuffer();
        }

        m_eventBuffer[m_eventCount++] = input_event{
            .type = type,
            .code = code,
            .value = value
        };
    }

    void UInputController::flushBuffer() {
        if (m_eventCount == 0) return;
        ::write(m_fd, m_eventBuffer.data(), m_eventCount * sizeof(input_event));
        m_eventCount = 0;
    }

    void UInputController::diffButtons(ControllerState const& newState) {
        auto prev = static_cast<uint16_t>(m_state.buttons);
        auto next = static_cast<uint16_t>(newState.buttons);
        uint16_t diff = prev ^ next;
        if (diff == 0) return;

        for (auto const& [bit, code] : BUTTON_MAPPING) {
            auto mask = static_cast<uint16_t>(bit);
            if (diff & mask) {
                this->pushEvent(EV_KEY, code, next & mask ? 1 : 0);
            }
        }
    }

    void UInputController::diffAxes(ControllerState const& newState) {
        auto axis = [&](int code, int16_t prev, int16_t next) {
            if (prev != next) {
                this->pushEvent(EV_ABS, code, next);
            }
        };

        axis(ABS_X, m_state.leftThumbX, newState.leftThumbX);
        axis(ABS_Y, m_state.leftThumbY, newState.leftThumbY);
        axis(ABS_RX, m_state.rightThumbX, newState.rightThumbX);
        axis(ABS_RY, m_state.rightThumbY, newState.rightThumbY);
        axis(ABS_Z, m_state.leftTrigger, newState.leftTrigger);
        axis(ABS_RZ, m_state.rightTrigger, newState.rightTrigger);
        axis(ABS_HAT1X, m_state.gyroX, newState.gyroX);
        axis(ABS_HAT1Y, m_state.gyroY, newState.gyroY);
        axis(ABS_HAT2X, m_state.gyroZ, newState.gyroZ);

        this->diffDPad(newState.buttons);
    }

    void UInputController::diffDPad(ControllerButtons newButtons) {
        auto has = [](ControllerButtons b, ControllerButtons f) -> bool {
            return static_cast<uint16_t>(b) & static_cast<uint16_t>(f);
        };

        int16_t newX = has(newButtons, ControllerButtons::DPadLeft) ? -1 :
                       has(newButtons, ControllerButtons::DPadRight) ? 1 : 0;
        int16_t newY = has(newButtons, ControllerButtons::DPadUp) ? -1 :
                       has(newButtons, ControllerButtons::DPadDown) ? 1 : 0;

        int16_t prevX = has(m_state.buttons, ControllerButtons::DPadLeft) ? -1 :
                        has(m_state.buttons, ControllerButtons::DPadRight) ? 1 : 0;
        int16_t prevY = has(m_state.buttons, ControllerButtons::DPadUp) ? -1 :
                        has(m_state.buttons, ControllerButtons::DPadDown) ? 1 : 0;

        if (prevX != newX) this->pushEvent(EV_ABS, ABS_HAT0X, newX);
        if (prevY != newY) this->pushEvent(EV_ABS, ABS_HAT0Y, newY);
    }
}
