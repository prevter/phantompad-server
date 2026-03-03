#pragma once
#include <functional>
#include <memory>
#include <Windows.h>
#include <Geode/Result.hpp>
#include <ViGEm/Client.h>

#include "../ControllerState.hpp"

namespace phant {
    class ViGEmController {
    public:
        using RumbleCallback = std::move_only_function<void(uint16_t strong, uint16_t weak)>;

        ViGEmController();
        ~ViGEmController();

        static geode::Result<std::unique_ptr<ViGEmController>> create(
            std::string_view name,
            RumbleCallback rumbleCallback,
            bool enableGyro = false
        );

        ViGEmController(ViGEmController const&) = delete;
        ViGEmController(ViGEmController&&) = delete;
        ViGEmController& operator=(ViGEmController const&) = delete;
        ViGEmController& operator=(ViGEmController&&) = delete;

        void setState(ControllerState const& state) const;

    private:
        static void CALLBACK ffCallback(
            PVIGEM_CLIENT, PVIGEM_TARGET,
            UCHAR largeMotor, UCHAR smallMotor,
            UCHAR, void* ctx
        );

        static USHORT remapButtons(ControllerButtons buttons);

    private:
        PVIGEM_CLIENT m_client = nullptr;
        PVIGEM_TARGET m_pad = nullptr;
        RumbleCallback m_rumbleCallback;
    };
}
