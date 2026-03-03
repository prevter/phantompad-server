#pragma once
#include <span>

#include "ClientEndpoint.hpp"
#include "Packets.hpp"

namespace phant {
    class PacketDispatcher {
        template <class T>
        static constexpr size_t packetMinSize() {
            if constexpr (requires { T::minSize; }) return T::minSize;
            else return sizeof(T);
        }

    public:
        void dispatch(this auto& self, ClientEndpoint const& from, std::span<uint8_t const> packet) {
            if (packet.empty()) return;

            #define DISPATCH(Name) \
                case PacketType::Name: \
                    if (packet.size() >= packetMinSize<Name##Packet>()) \
                        self.on##Name(from, *reinterpret_cast<Name##Packet const*>(packet.data())); \
                    return;

            switch (static_cast<PacketType>(packet[0])) {
                DISPATCH(DiscoveryRequest)
                DISPATCH(ConnectRequest)
                DISPATCH(Disconnect)
                DISPATCH(Ping)
                DISPATCH(Input)

                default: self.onUnknownPacket(from, packet);
                    break;
            }

            #undef DISPATCH
        }
    };
}
