#include "WinSocket.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <fmt/format.h>

#pragma comment(lib, "ws2_32.lib")

namespace phant {
    static std::string getLastErrorMessage() {
        int errorCode = WSAGetLastError();
        char* messageBuffer = nullptr;
        size_t size = FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr, errorCode,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPSTR>(&messageBuffer),
            0, nullptr
        );

        std::string message(messageBuffer, size);
        LocalFree(messageBuffer);
        return message;
    }

    WinSocket::WinSocket(uint16_t port) : m_port(port) {}

    WinSocket::~WinSocket() { this->close(); }

    geode::Result<> WinSocket::open() {
        WSADATA wsaData;
        int wsaResult = ::WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (wsaResult != 0) {
            return geode::Err(fmt::format("WSAStartup failed: {}", getLastErrorMessage()));
        }

        SOCKET sock = ::WSASocketW(AF_INET, SOCK_DGRAM, IPPROTO_UDP, nullptr, 0, WSA_FLAG_OVERLAPPED);
        if (sock == INVALID_SOCKET) {
            ::WSACleanup();
            return geode::Err(fmt::format("Failed to create socket: {}", getLastErrorMessage()));
        }

        u_long nonBlocking = 1;
        if (::ioctlsocket(sock, FIONBIO, &nonBlocking) == SOCKET_ERROR) {
            ::closesocket(sock);
            ::WSACleanup();
            return geode::Err(fmt::format("Failed to set non-blocking mode: {}", getLastErrorMessage()));
        }

        int opt = 1;
        if (
            ::setsockopt(
                sock, SOL_SOCKET, SO_REUSEADDR,
                reinterpret_cast<char const*>(&opt),
                sizeof(opt)
            ) == SOCKET_ERROR
        ) {
            ::closesocket(sock);
            ::WSACleanup();
            return geode::Err(fmt::format("Failed to set SO_REUSEADDR: {}", getLastErrorMessage()));
        }

        int rcvBufSize = 1024 * 1024; // 1 MB
        if (
            ::setsockopt(
                sock, SOL_SOCKET, SO_RCVBUF,
                reinterpret_cast<char const*>(&rcvBufSize),
                sizeof(rcvBufSize)
            ) == SOCKET_ERROR
        ) {
            ::closesocket(sock);
            ::WSACleanup();
            return geode::Err(fmt::format("Failed to set SO_RCVBUF: {}", getLastErrorMessage()));
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(m_port);
        if (::bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
            ::closesocket(sock);
            ::WSACleanup();
            return geode::Err(fmt::format("Failed to bind socket: {}", getLastErrorMessage()));
        }

        m_socket = sock;

        m_wakeEvent = WSACreateEvent();
        if (m_wakeEvent == INVALID_HANDLE_VALUE) {
            ::closesocket(sock);
            ::WSACleanup();
            return geode::Err(fmt::format("Failed to create wake event: {}", getLastErrorMessage()));
        }

        m_socketEvent = WSACreateEvent();
        if (m_socketEvent == INVALID_HANDLE_VALUE) {
            ::CloseHandle(m_wakeEvent);
            ::closesocket(sock);
            ::WSACleanup();
            return geode::Err(fmt::format("Failed to create socket event: {}", getLastErrorMessage()));
        }

        ::WSAEventSelect(m_socket, m_socketEvent, FD_READ);

        return geode::Ok();
    }

    void WinSocket::close() {
        if (m_wakeEvent != WSA_INVALID_EVENT) {
            ::CloseHandle(std::exchange(m_wakeEvent, WSA_INVALID_EVENT));
        }

        if (m_socketEvent != WSA_INVALID_EVENT) {
            ::CloseHandle(std::exchange(m_socketEvent, WSA_INVALID_EVENT));
        }

        if (m_socket != INVALID_SOCKET) {
            ::closesocket(std::exchange(m_socket, INVALID_SOCKET));
            ::WSACleanup();
        }
    }

    geode::Result<bool> WinSocket::poll(int timeoutMs) const {
        WSAEVENT events[2] = { m_socketEvent, m_wakeEvent };
        DWORD result = ::WSAWaitForMultipleEvents(2, events, FALSE, timeoutMs, FALSE);

        if (result == WSA_WAIT_FAILED) {
            return geode::Err(fmt::format("WSAWaitForMultipleEvents failed: {}", getLastErrorMessage()));
        }

        if (result == WSA_WAIT_TIMEOUT || result == WSA_WAIT_EVENT_0 + 1) {
            return geode::Ok(false);
        }

        return geode::Ok(true);
    }

    geode::Result<> WinSocket::sendTo(ClientEndpoint const& to, std::span<uint8_t const> data) const {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = to.ip;
        addr.sin_port = to.port;

        int result = ::sendto(
            m_socket,
            reinterpret_cast<char const*>(data.data()),
            static_cast<int>(data.size()),
            0,
            reinterpret_cast<sockaddr*>(&addr),
            sizeof(addr)
        );

        if (result == SOCKET_ERROR) {
            return geode::Err(fmt::format("sendto failed: {}", getLastErrorMessage()));
        }

        return geode::Ok();
    }

    geode::Result<std::optional<ReceiveResult>> WinSocket::receiveTo(std::span<uint8_t> buffer) const {
        sockaddr_in from{};
        int fromLen = sizeof(from);
        int result = ::recvfrom(
            m_socket,
            reinterpret_cast<char*>(buffer.data()),
            static_cast<int>(buffer.size()),
            0,
            reinterpret_cast<sockaddr*>(&from),
            &fromLen
        );

        if (result == SOCKET_ERROR) {
            int errorCode = ::WSAGetLastError();
            if (errorCode == WSAEWOULDBLOCK) {
                return geode::Ok(std::nullopt);
            }
            return geode::Err(fmt::format("recvfrom failed: {}", getLastErrorMessage()));
        }

        return geode::Ok(ReceiveResult{
            {static_cast<uint32_t>(from.sin_addr.s_addr), from.sin_port},
            static_cast<size_t>(result)
        });
    }

    void WinSocket::wakeup() const {
        ::WSASetEvent(m_wakeEvent);
    }
}
