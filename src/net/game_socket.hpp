#pragma once
#include <defs.hpp>

#include "tcp_socket.hpp"
#include "udp_socket.hpp"

#include <data/packets/packet.hpp>
#include <crypto/box.hpp>

class GameSocket {
public:
    enum class PollResult {
        None, Tcp, Udp, Both
    };

    GameSocket();
    ~GameSocket();

    std::shared_ptr<Packet> recvPacket(bool onActive);
    void sendPacket(std::shared_ptr<Packet> packet);
    void sendPacketTo(std::shared_ptr<Packet> packet, const std::string_view address, unsigned short port);

    ByteBuffer serializePacket(Packet* packet);

    void cleanupBox();
    void createBox();

    Result<PollResult> pollEither(int msDelay);

private:
    friend class NetworkManager;

    TcpSocket tcpSocket;
    UdpSocket udpSocket;

    std::unique_ptr<CryptoBox> box;
    util::data::byte* buffer;
};