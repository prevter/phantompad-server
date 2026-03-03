#pragma once
#include <cstdint>
#include <span>
#include <Geode/Result.hpp>

#include "../ClientEndpoint.hpp"

namespace phant {
    class WinSocket {
    public:
        explicit WinSocket(uint16_t port);
        ~WinSocket();

        geode::Result<> open();
        void close();

        geode::Result<bool> poll(int timeoutMs) const;
        geode::Result<> sendTo(ClientEndpoint const& to, std::span<uint8_t const> data) const;
        geode::Result<std::optional<ReceiveResult>> receiveTo(std::span<uint8_t> buffer) const;

        void wakeup() const;

    private:
        uintptr_t m_socket = static_cast<uintptr_t>(-1);
        void* m_socketEvent = nullptr;
        void* m_wakeEvent = nullptr;
        uint16_t m_port;
    };
}