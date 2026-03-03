#pragma once
#include <cstdint>

#include "ControllerState.hpp"

namespace phant {
    constexpr uint16_t PROTOCOL_VERSION = 1;

    enum class PacketType : uint8_t {
        DiscoveryRequest  = 0x01, // Client -> Server (local network scan)
        DiscoveryResponse = 0x02, // Server -> Client (local network scan response)

        ConnectRequest  = 0x03, // Client -> Server (request to connect)
        ConnectResponse = 0x04, // Server -> Client (response to connect request)

        Disconnect = 0x05, // Client <-> Server (disconnect notification)
        Ping       = 0x06, // Client -> Server (latency check)
        Pong       = 0x07, // Server -> Client (response to ping)

        Input          = 0x08, // Client -> Server (player input)
        HapticFeedback = 0x09, // Server -> Client (haptic feedback command)
    };

    enum class ServerIcon : uint8_t {
        Generic   = 0x00,
        Desktop   = 0x01,
        Laptop    = 0x02,
        HTPC      = 0x03,
        SteamDeck = 0x04,
    };

    enum class ConnectStatus : uint8_t {
        Success               = 0x00, // Connection successful
        ServerFull            = 0x01, // Server has reached max client capacity
        VersionMismatch       = 0x02, // Client version is incompatible with server
        ControllerInitFailure = 0x03, // Server failed to initialize controller for client
        ViGEmNotFound         = 0x04, // (Windows) ViGEmBus driver not found or not working
    };

    #pragma pack(push, 1)

    struct DiscoveryRequestPacket {
        PacketType type = PacketType::DiscoveryRequest;
        uint64_t timestamp;
    };

    struct DiscoveryResponsePacket {
        PacketType type = PacketType::DiscoveryResponse;
        ServerIcon icon = ServerIcon::Generic;
        uint64_t timestamp; // copied from client's request for latency calculation
        uint8_t nameLength;
        char name[255]{};

        static constexpr size_t minSize = 11; // type + icon + timestamp + nameLength
        constexpr size_t wireSize() const { return minSize + nameLength; }
    };

    struct ConnectRequestPacket {
        PacketType type = PacketType::ConnectRequest;
        uint16_t clientVersion;
        uint8_t nameLength;
        char name[255]{};

        static constexpr size_t minSize = 4; // type + version + nameLength
        constexpr size_t wireSize() const { return minSize + nameLength; }
    };

    struct ConnectResponsePacket {
        PacketType type = PacketType::ConnectResponse;
        ConnectStatus status{}; // 0 for success, non-zero for failure
        uint8_t slotId = 0;     // assigned device slot ID (0-7)
        uint8_t nameLength;
        char name[255]{};

        static constexpr size_t minSize = 4; // type + status + slotId + nameLength
        constexpr size_t wireSize() const { return minSize + nameLength; }
    };

    struct DisconnectPacket {
        PacketType type = PacketType::Disconnect;
    };

    struct PingPacket {
        PacketType type = PacketType::Ping;
        uint64_t timestamp;
    };

    struct PongPacket {
        PacketType type = PacketType::Pong;
        uint64_t timestamp;
    };

    struct InputPacket {
        PacketType type = PacketType::Input;
        uint32_t sequence;
        ControllerState state;
    };

    struct HapticFeedbackPacket {
        PacketType type = PacketType::HapticFeedback;
        uint16_t strongMagnitude;
        uint16_t weakMagnitude;
    };

    #pragma pack(pop)
}
