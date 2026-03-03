#pragma once
#include <array>
#include <atomic>
#include <chrono>
#include <Geode/Result.hpp>

#include "PacketDispatcher.hpp"
#include "Utils.hpp"

namespace phant {
    using geode::Result;
    using geode::Ok, geode::Err;

    using Clock = std::chrono::steady_clock;
    using Timepoint = Clock::time_point;

    template <class Socket, class Controller>
    class Server : public PacketDispatcher {
    public:
        static constexpr uint16_t PORT = 35700;
        static constexpr uint8_t MAX_CLIENTS = 8;
        static constexpr auto CLIENT_TIMEOUT = std::chrono::seconds(30);
        static constexpr auto TIMEOUT_CHECK_MS = 5000;

        struct ClientSession {
            std::unique_ptr<Controller> controller;
            std::string name;
            ClientEndpoint endpoint{};
            Timepoint lastSeen = Clock::now();
            uint32_t lastSequence = 0;
            ControllerState state{};
            uint8_t slot{};
            bool active = false;

            void disable() {
                active = false;
                controller.reset();
            }
        };

        Server() : m_socket(PORT),
            m_serverName(util::getDefaultServerName()),
            m_serverIcon(util::detectServerIcon()) {}

        ~Server() {
            this->stop();
            m_socket.close();
        }

        Result<> run() {
            GEODE_UNWRAP(m_socket.open());

            fmt::println("Server '{}' running on port {}", m_serverName, PORT);

            m_running = true;
            while (m_running) {
                GEODE_UNWRAP_INTO(bool hasActivity, m_socket.poll(TIMEOUT_CHECK_MS));
                if (hasActivity) {
                    while (true) {
                        auto packetResult = m_socket.receiveTo(m_recvBuffer);
                        if (packetResult.isErr()) {
                            fmt::println(stderr, "Failed to receive packet: {}", packetResult.unwrapErr());
                            continue;
                        }

                        if (!packetResult.unwrap().has_value()) break; // no more packets to read

                        auto [from, size] = std::move(packetResult).unwrap().value();
                        this->dispatch(from, std::span<uint8_t const>{m_recvBuffer.data(), size});
                    }
                }

                this->checkTimedOutClients();
            }

            return Ok();
        }

        void stop() {
            m_running = false;
            m_socket.wakeup();
        }

        void checkTimedOutClients() {
            auto now = Clock::now();
            for (size_t i = 0; i < MAX_CLIENTS; ++i) {
                if (m_clients[i].active && now - m_clients[i].lastSeen > CLIENT_TIMEOUT) {
                    fmt::println("Client {} timed out (slot {})", m_clients[i].endpoint.toString(), i);
                    m_clientMap.erase(m_clients[i].endpoint);
                    m_clients[i].disable();
                }
            }
        }

        template <class PacketType>
        Result<> send(ClientEndpoint const& to, PacketType const& packet) {
            if constexpr (requires{ packet.wireSize(); }) {
                return m_socket.sendTo(to, std::span{
                    reinterpret_cast<uint8_t const*>(&packet),
                    packet.wireSize()
                });
            } else {
                return m_socket.sendTo(to, std::span{
                    reinterpret_cast<uint8_t const*>(&packet),
                    sizeof(PacketType)
                });
            }
        }

        /// == Packet handlers == ///

        void onDiscoveryRequest(ClientEndpoint const& from, DiscoveryRequestPacket const& packet) {
            DiscoveryResponsePacket response{
                .icon = m_serverIcon,
                .timestamp = packet.timestamp,
                .nameLength = std::min<uint8_t>(m_serverName.size(), sizeof(response.name))
            };

            std::memcpy(response.name, m_serverName.data(), response.nameLength);
            (void) this->send(from, response);
        }

        void onConnectRequest(ClientEndpoint const& from, ConnectRequestPacket const& packet) {
            ConnectResponsePacket response{
                .nameLength = std::min<uint8_t>(m_serverName.size(), sizeof(response.name))
            };

            std::memcpy(response.name, m_serverName.data(), response.nameLength);

            if (m_clientMap.size() >= MAX_CLIENTS) {
                response.status = 1;
            } else {
                uint8_t slotId = 0;
                for (; slotId < MAX_CLIENTS; ++slotId) {
                    if (!m_clients[slotId].active) break;
                }

                auto controllerResult = Controller::create(
                    fmt::format(
                        "Phantom Controller - {} #{}",
                        std::string_view(packet.name, packet.nameLength),
                        slotId + 1
                    ),
                    [this, slotId](uint16_t strong, uint16_t weak) {
                        (void) this->send(m_clients[slotId].endpoint, HapticFeedbackPacket{
                            .strongMagnitude = strong,
                            .weakMagnitude = weak
                        });
                    }
                );

                if (controllerResult) {
                    response.status = 0;
                    response.slotId = slotId;

                    // store client session
                    m_clients[slotId] = ClientSession{
                        .controller = std::move(controllerResult).unwrap(),
                        .name = std::string(packet.name, packet.nameLength),
                        .endpoint = from,
                        .lastSeen = Clock::now(),
                        .lastSequence = 0,
                        .state = {},
                        .slot = slotId,
                        .active = true
                    };

                    m_clientMap[from] = slotId;
                    fmt::println("Client '{}' connected (slot {})", m_clients[slotId].name, slotId);
                } else {
                    response.status = 2;
                    fmt::println(stderr, "Failed to create controller for client {}: {}", from.toString(), controllerResult.unwrapErr());
                }
            }

            (void) this->send(from, response);
        }

        void onDisconnect(ClientEndpoint const& from, DisconnectPacket const&) {
            auto it = m_clientMap.find(from);
            if (it != m_clientMap.end()) {
                fmt::println("Client '{}' disconnected (slot {})", m_clients[it->second].name, it->second);
                m_clients[it->second].disable();
                m_clientMap.erase(it);
            }
        }

        void onPing(ClientEndpoint const& from, PingPacket const& packet) {
            auto it = m_clientMap.find(from);
            if (it != m_clientMap.end()) {
                m_clients[it->second].lastSeen = Clock::now();
                (void) this->send(from, PongPacket{ .timestamp = packet.timestamp });
            } else {
                // stale client state, respond with disconnect
                fmt::println(stderr, "Received ping from unknown client {}, sending disconnect", from.toString());
                (void) this->send(from, DisconnectPacket{});
            }
        }

        void onInput(ClientEndpoint const& from, InputPacket const& packet) {
            auto it = m_clientMap.find(from);
            if (it != m_clientMap.end()) {
                ClientSession& session = m_clients[it->second];
                session.lastSeen = Clock::now();

                if (packet.sequence > session.lastSequence) {
                    session.lastSequence = packet.sequence;
                    session.state = packet.state;
                    session.controller->setState(packet.state);
                }
            } else {
                // stale client state, respond with disconnect
                fmt::println(stderr, "Received input from unknown client {}, sending disconnect", from.toString());
                (void) this->send(from, DisconnectPacket{});
            }
        }

        void onUnknownPacket(ClientEndpoint const& from, std::span<uint8_t const> packet) {
            fmt::println(stderr, "Received unknown packet from {}: {}", from.toString(), packet.size());
        }

    private:
        Socket m_socket;
        std::array<ClientSession, MAX_CLIENTS> m_clients{};
        std::unordered_map<ClientEndpoint, uint8_t, ClientEndpointHash> m_clientMap;
        std::array<uint8_t, 65536> m_recvBuffer{};
        std::atomic<bool> m_running = false;

        std::string m_serverName;
        ServerIcon m_serverIcon;
    };
}
