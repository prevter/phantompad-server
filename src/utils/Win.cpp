#include "../Utils.hpp"

#include <Windows.h>

namespace phant::util {
    std::string getDefaultServerName() {
        wchar_t buffer[MAX_PATH];
        DWORD size = sizeof(buffer);
        if (!::GetComputerNameW(buffer, &size)) {
            return "Unknown PC";
        }

        int len = WideCharToMultiByte(CP_UTF8, 0, buffer, -1, nullptr, 0, nullptr, nullptr);
        std::string result(len - 1, '\0');
        WideCharToMultiByte(CP_UTF8, 0, buffer, -1, result.data(), len, nullptr, nullptr);
        return result;
    }

    static bool isLaptop() {
        SYSTEM_POWER_STATUS status;
        if (!::GetSystemPowerStatus(&status)) {
            return false;
        }
        return status.BatteryFlag != 128; // No system battery
    }

    ServerIcon detectServerIcon() {
        if (isLaptop()) return ServerIcon::Laptop;
        return ServerIcon::Desktop;
    }
}