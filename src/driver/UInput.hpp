#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <thread>
#include <Geode/Result.hpp>

#include <fcntl.h>
#include <linux/input.h>

#include "../ControllerState.hpp"

namespace phant {
    struct ButtonMapping {
        ControllerButtons bit;
        uint16_t linuxCode;
    };

    constexpr auto BUTTON_MAPPING = std::to_array<ButtonMapping>({
        {ControllerButtons::A, BTN_A},
        {ControllerButtons::B, BTN_B},
        {ControllerButtons::X, BTN_X},
        {ControllerButtons::Y, BTN_Y},
        {ControllerButtons::LeftBumper, BTN_TL},
        {ControllerButtons::RightBumper, BTN_TR},
        {ControllerButtons::Back, BTN_SELECT},
        {ControllerButtons::Start, BTN_START},
        {ControllerButtons::Guide, BTN_MODE},
        {ControllerButtons::LeftThumb, BTN_THUMBL},
        {ControllerButtons::RightThumb, BTN_THUMBR},
    });

    struct AxisSetup {
        int code;
        int32_t min, max;
        int32_t flat = 0, fuzz = 0;
    };

    constexpr auto AXIS_SETUP = std::to_array<AxisSetup>({
        {ABS_X, -32767, 32767},  // leftThumbX
        {ABS_Y, -32767, 32767},  // leftThumbY
        {ABS_RX, -32767, 32767}, // rightThumbX
        {ABS_RY, -32767, 32767}, // rightThumbY

        {ABS_Z, 0, 255},  // leftTrigger
        {ABS_RZ, 0, 255}, // rightTrigger

        {ABS_HAT0X, -1, 1}, // dpad X
        {ABS_HAT0Y, -1, 1}, // dpad Y

        {ABS_HAT1X, -32767, 32767}, // gyroX
        {ABS_HAT1Y, -32767, 32767}, // gyroY
        {ABS_HAT2X, -32767, 32767}, // gyroZ
    });

    constexpr size_t AXIS_COUNT_NO_GYRO = 8;
    constexpr size_t AXIS_COUNT_GYRO = AXIS_SETUP.size();

    class UInputController {
    public:
        static constexpr int MAX_FF_EFFECTS = 4;
        static constexpr int EVENT_BUFFER_SIZE = 64;
        using RumbleCallback = std::move_only_function<void(uint16_t strong, uint16_t weak)>;

        UInputController();
        ~UInputController();

        static geode::Result<std::unique_ptr<UInputController>> create(
            std::string_view name,
            RumbleCallback rumbleCallback,
            bool enableGyro = false
        );

        UInputController(UInputController const&) = delete;
        UInputController(UInputController&&) noexcept = delete;
        UInputController& operator=(UInputController const&) = delete;
        UInputController& operator=(UInputController&&) noexcept = delete;

        void setState(ControllerState const& state);

    private:
        static void waitForDevice(int fd);
        void workerThread();
        void handleFFControl(input_event const& event);
        void handleFFPlay(input_event const& event);
        void sync();

        void pushEvent(uint16_t type, uint16_t code, int32_t value);
        void flushBuffer();

        void diffButtons(ControllerState const& newState);
        void diffAxes(ControllerState const& newState);
        void diffDPad(ControllerButtons newButtons);

    private:
        int m_fd = -1;
        ControllerState m_state{};

        std::array<input_event, EVENT_BUFFER_SIZE> m_eventBuffer{};
        size_t m_eventCount = 0;

        std::thread m_ffThread;
        std::atomic<bool> m_stopFF{false};
        std::unordered_map<int, ff_effect> m_ffEffects;
        RumbleCallback m_rumbleCallback;
    };
}