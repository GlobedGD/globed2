#pragma once
#include "udp_socket.hpp"
#include <data/packets/packet.hpp>
#include <crypto/box.hpp>

class GameSocket : public UdpSocket {
public:
    GameSocket();
    ~GameSocket();

    std::shared_ptr<Packet> recvPacket();
    void sendPacket(std::shared_ptr<Packet> packet);
    void sendPacketTo(std::shared_ptr<Packet> packet, const std::string& address, unsigned short port);

    ByteBuffer serializePacket(Packet* packet);

    void cleanupBox();
    void createBox();

private:
    friend class NetworkManager;

    std::unique_ptr<CryptoBox> box;
    util::data::byte* buffer;
};