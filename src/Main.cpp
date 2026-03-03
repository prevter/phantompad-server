#include <csignal>
#include <fmt/format.h>

#include "Server.hpp"

using namespace phant;

#ifdef _WIN32
    #include "driver/ViGEm.hpp"
    #include "socket/WinSocket.hpp"
    using SocketType = WinSocket;
    using ControllerType = ViGEmController;
#else
    #include "driver/UInput.hpp"
    #include "socket/UnixSocket.hpp"
    using SocketType = UnixSocket;
    using ControllerType = UInputController;
#endif

static Server<SocketType, ControllerType> server;

#ifdef _WIN32

#include <Windows.h>

static constexpr wchar_t SERVICE_NAME[] = L"PhantomPad";
static SERVICE_STATUS_HANDLE g_statusHandle = nullptr;
static SERVICE_STATUS g_serviceStatus = {
    .dwServiceType = SERVICE_WIN32_OWN_PROCESS,
    .dwCurrentState = SERVICE_START_PENDING,
    .dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN,
};

static void reportServiceStatus(DWORD state, DWORD exitCode = NO_ERROR) {
    g_serviceStatus.dwCurrentState = state;
    g_serviceStatus.dwWin32ExitCode = exitCode;
    SetServiceStatus(g_statusHandle, &g_serviceStatus);
}

static void WINAPI serviceCtrlHandler(DWORD ctrl) {
    if (ctrl == SERVICE_CONTROL_STOP || ctrl == SERVICE_CONTROL_SHUTDOWN) {
        reportServiceStatus(SERVICE_STOP_PENDING);
        server.stop();
    }
}

static void WINAPI serviceMain(DWORD, LPWSTR*) {
    g_statusHandle = RegisterServiceCtrlHandlerW(SERVICE_NAME, serviceCtrlHandler);
    if (!g_statusHandle) return;

    reportServiceStatus(SERVICE_RUNNING);

    auto result = server.run();
    reportServiceStatus(SERVICE_STOPPED, result.isErr() ? ERROR_SERVICE_SPECIFIC_ERROR : NO_ERROR);
}

static int runAsService() {
    SERVICE_TABLE_ENTRYW serviceTable[] = {
        { const_cast<LPWSTR>(SERVICE_NAME), serviceMain },
        { nullptr, nullptr }
    };

    if (!StartServiceCtrlDispatcherW(serviceTable)) {
        if (GetLastError() == ERROR_FAILED_SERVICE_CONTROLLER_CONNECT) {
            return -1; // Not running as a service
        }
    }

    return 0;
}

#endif

void signalHandler(int) { server.stop(); }

int main() {
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    #ifdef _WIN32
    int serviceResult = runAsService();
    if (serviceResult != -1) {
        return serviceResult;
    }
    #endif

    auto result = server.run();
    fmt::println("Server stopped");

    if (result.isErr()) {
        fmt::println(stderr, "Error: {}", result.unwrapErr());
        return 1;
    }

    return 0;
}