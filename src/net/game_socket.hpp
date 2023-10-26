#pragma once
#include "udp_socket.hpp"
#include <data/packets/packet.hpp>
#include <crypto/box.hpp>

class GameSocket : public UdpSocket {
public:
    GameSocket();

    std::shared_ptr<Packet> recvPacket();
    void sendPacket(Packet* packet);

private:
    friend class NetworkManager;

    CryptoBox box;
    util::data::byte* buffer;
};