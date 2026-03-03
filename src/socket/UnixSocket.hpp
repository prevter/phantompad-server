#pragma once
#include <cstdint>
#include <span>
#include <Geode/Result.hpp>

#include "../ClientEndpoint.hpp"

namespace phant {
    class UnixSocket {
    public:
        explicit UnixSocket(uint16_t port);
        ~UnixSocket();

        geode::Result<> open();
        void close();

        geode::Result<bool> poll(int timeoutMs) const;
        geode::Result<> sendTo(ClientEndpoint const& to, std::span<uint8_t const> data) const;
        geode::Result<std::optional<ReceiveResult>> receiveTo(std::span<uint8_t> buffer) const;

        void wakeup() const;

    private:
        int m_socketFd = -1;
        int m_epollFd = -1;
        int m_eventFd = -1;
        uint16_t m_port;
    };
}