#pragma once

#include "tcp_socket.hpp"
#include "udp_socket.hpp"

#include <data/packets/packet.hpp>
#include <crypto/box.hpp>

class GameSocket {
public:
    GameSocket();
    ~GameSocket();

    // Receive a packet on either the TCP or the UDP connection. Will block until a packet has been received.
    Result<std::shared_ptr<Packet>> recvPacket(bool onTcpConnection, bool& fromConnected);

    // Same as `recvPacket` but with an optional timeout. If timeout is reached, `timedOut` is set to true and function returns an error.
    Result<std::shared_ptr<Packet>> recvPacket(int timeoutMs, bool& fromConnected, bool& timedOut);

    // Send a packet to the current active connection.
    Result<> sendPacket(std::shared_ptr<Packet> packet);

    // Send a UDP packet to a specific address
    Result<> sendPacketTo(std::shared_ptr<Packet> packet, const std::string_view address, unsigned short port);

    bool isConnected();

    Result<> connect(const std::string_view address, unsigned short port);
    void disconnect();

    void cleanupBox();
    void createBox();

    enum class PollResult {
        None, Tcp, Udp, Both
    };

    Result<PollResult> poll(int timeoutMs);

private:
    friend class NetworkManager;

    TcpSocket tcpSocket;
    UdpSocket udpSocket;

    std::unique_ptr<CryptoBox> cryptoBox;
    util::data::byte* buffer;

    Result<> serializePacket(Packet* packet, ByteBuffer& buffer);
};