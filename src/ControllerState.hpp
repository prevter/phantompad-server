#pragma once

#include <array>
#include <cstdint>

namespace phant {
    enum class ControllerButtons : uint16_t {
        A           = 1 << 0,
        B           = 1 << 1,
        X           = 1 << 2,
        Y           = 1 << 3,
        LeftBumper  = 1 << 4,
        RightBumper = 1 << 5,
        Back        = 1 << 6,
        Start       = 1 << 7,
        Guide       = 1 << 8,
        LeftThumb   = 1 << 9,
        RightThumb  = 1 << 10,
        DPadUp      = 1 << 11,
        DPadDown    = 1 << 12,
        DPadLeft    = 1 << 13,
        DPadRight   = 1 << 14,
        Unused      = 1 << 15,
    };

    #pragma pack(push, 1)

    struct ControllerState {
        ControllerButtons buttons{};
        int16_t leftThumbX = 0;
        int16_t leftThumbY = 0;
        int16_t rightThumbX = 0;
        int16_t rightThumbY = 0;
        int16_t gyroX = 0;
        int16_t gyroY = 0;
        int16_t gyroZ = 0;
        uint8_t leftTrigger = 0;
        uint8_t rightTrigger = 0;
    };

    #pragma pack(pop)
}
