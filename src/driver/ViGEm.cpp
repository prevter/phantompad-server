#include "ViGEm.hpp"

#include <fmt/format.h>

namespace phant {
    ViGEmController::ViGEmController() = default;
    ViGEmController::~ViGEmController() {
        if (m_client && m_pad) vigem_target_remove(m_client, m_pad);
        if (m_pad) vigem_target_free(m_pad);
        if (m_client) {
            vigem_disconnect(m_client);
            vigem_free(m_client);
        }
    }

    geode::Result<std::unique_ptr<ViGEmController>> ViGEmController::create(
        std::string_view name,
        RumbleCallback rumbleCallback,
        bool enableGyro
    ) {
        auto client = vigem_alloc();
        if (!client) {
            return geode::Err("Failed to allocate ViGEm client");
        }

        auto result = vigem_connect(client);
        if (!VIGEM_SUCCESS(result)) {
            vigem_free(client);
            return geode::Err(fmt::format(
                "ViGEmBus not found (0x{:08X}). Please install it from "
                "https://github.com/nefarius/ViGEmBus/releases/latest",
                static_cast<uint32_t>(result)
            ));
        }

        auto pad = vigem_target_x360_alloc();
        if (!pad) {
            vigem_disconnect(client);
            vigem_free(client);
            return geode::Err("Failed to allocate ViGEm target");
        }

        vigem_target_set_vid(pad, 0x045E); // Microsoft
        vigem_target_set_pid(pad, enableGyro ? 0x028E : 0x028F); // Xbox One S / Xbox One
        vigem_target_add(client, pad);

        auto controller = std::make_unique<ViGEmController>();
        controller->m_client = client;
        controller->m_pad = pad;
        controller->m_rumbleCallback = std::move(rumbleCallback);

        vigem_target_x360_register_notification(
            client, pad,
            &ViGEmController::ffCallback,
            controller.get()
        );

        return geode::Ok(std::move(controller));
    }

    void ViGEmController::setState(ControllerState const& state) const {
        vigem_target_x360_update(m_client, m_pad, XUSB_REPORT{
            .wButtons = remapButtons(state.buttons),
            .bLeftTrigger = state.leftTrigger,
            .bRightTrigger = state.rightTrigger,
            .sThumbLX = state.leftThumbX,
            .sThumbLY = static_cast<SHORT>(-state.leftThumbY),
            .sThumbRX = state.rightThumbX,
            .sThumbRY = static_cast<SHORT>(-state.rightThumbY),
        });
    }

    void ViGEmController::ffCallback(
        PVIGEM_CLIENT, PVIGEM_TARGET,
        UCHAR largeMotor, UCHAR smallMotor,
        UCHAR, void* ctx
    ) {
        static_cast<ViGEmController*>(ctx)->m_rumbleCallback(largeMotor * 257, smallMotor * 257);
    }

    USHORT ViGEmController::remapButtons(ControllerButtons buttons) {
        auto has = [&](ControllerButtons f) {
            return static_cast<uint16_t>(buttons) & static_cast<uint16_t>(f);
        };

        USHORT result = 0;
        if (has(ControllerButtons::A)) result |= XUSB_GAMEPAD_A;
        if (has(ControllerButtons::B)) result |= XUSB_GAMEPAD_B;
        if (has(ControllerButtons::X)) result |= XUSB_GAMEPAD_X;
        if (has(ControllerButtons::Y)) result |= XUSB_GAMEPAD_Y;
        if (has(ControllerButtons::LeftBumper)) result |= XUSB_GAMEPAD_LEFT_SHOULDER;
        if (has(ControllerButtons::RightBumper)) result |= XUSB_GAMEPAD_RIGHT_SHOULDER;
        if (has(ControllerButtons::Back)) result |= XUSB_GAMEPAD_BACK;
        if (has(ControllerButtons::Start)) result |= XUSB_GAMEPAD_START;
        if (has(ControllerButtons::Guide)) result |= XUSB_GAMEPAD_GUIDE;
        if (has(ControllerButtons::LeftThumb)) result |= XUSB_GAMEPAD_LEFT_THUMB;
        if (has(ControllerButtons::RightThumb)) result |= XUSB_GAMEPAD_RIGHT_THUMB;
        if (has(ControllerButtons::DPadUp)) result |= XUSB_GAMEPAD_DPAD_UP;
        if (has(ControllerButtons::DPadDown)) result |= XUSB_GAMEPAD_DPAD_DOWN;
        if (has(ControllerButtons::DPadLeft)) result |= XUSB_GAMEPAD_DPAD_LEFT;
        if (has(ControllerButtons::DPadRight)) result |= XUSB_GAMEPAD_DPAD_RIGHT;
        return result;
    }
}
