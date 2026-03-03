#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <fmt/format.h>

namespace phant {
    struct ClientEndpoint {
        uint32_t ip{};
        uint16_t port{};

        constexpr ClientEndpoint() noexcept = default;
        constexpr ClientEndpoint(uint32_t ip, uint16_t port) noexcept : ip{ip}, port{port} {}

        constexpr bool operator==(ClientEndpoint const& other) const {
            return ip == other.ip && port == other.port;
        }

        std::string toString() const {
            uint8_t bytes[4];
            bytes[0] = ip & 0xFF;
            bytes[1] = ip >> 8 & 0xFF;
            bytes[2] = ip >> 16 & 0xFF;
            bytes[3] = ip >> 24 & 0xFF;
            return fmt::format("{}.{}.{}.{}:{}", bytes[0], bytes[1], bytes[2], bytes[3], port);
        }
    };

    struct ClientEndpointHash {
        size_t operator()(ClientEndpoint const& endpoint) const noexcept {
            return std::hash<uint64_t>()(static_cast<uint64_t>(endpoint.ip) << 16 | endpoint.port);
        }
    };

    struct ReceiveResult {
        ClientEndpoint from;
        size_t size;
    };
}
