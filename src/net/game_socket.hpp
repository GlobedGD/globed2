#pragma once

#include "address.hpp"
#include "udp_socket.hpp"
#include "tcp_socket.hpp"
#include "udp_frame_buffer.hpp"

#include <data/packets/packet.hpp>
#include <crypto/box.hpp>

class GLOBED_DLL GameSocket {
    static constexpr uint8_t MARKER_CONN_INITIAL = 0xe0;
    static constexpr uint8_t MARKER_CONN_RECOVERY = 0xe1;

    static constexpr uint8_t MARKER_UDP_PACKET = 0xb1;
    static constexpr uint8_t MARKER_UDP_FRAME = 0xa7;

public:
    GameSocket();
    ~GameSocket();

    Result<> connect(const NetworkAddress& address, bool isRecovering);
    Result<> connectWithRelay(const NetworkAddress& address, const NetworkAddress& relayAddress, bool isRecovering);
    void disconnect();
    bool isConnected();

    struct ReceivedPacket {
        std::shared_ptr<Packet> packet;
        bool fromConnected;
    };

    enum class Protocol {
        Unspecified, Tcp, Udp
    };

    // Try to receive a packet on the TCP socket
    Result<std::shared_ptr<Packet>> recvPacketTCP();

    // Try to receive a packet on the UDP socket
    Result<std::optional<ReceivedPacket>> recvPacketUDP(bool skipMarker = false);

    // Try to receive a packet
    Result<ReceivedPacket> recvPacket();

    // Try to receive a packet, returns "timed out" if timeout is reached.
    Result<ReceivedPacket> recvPacket(int timeoutMs);

    // Receive raw bytes from a TCP socket. if size is not specified, reads 4 bytes from the socket and uses that as the length.
    // Blocks until either an error occurs, the timeout expires or the data is ready.
    Result<std::vector<uint8_t>> recvRawTcpData(size_t size, int timeoutMs);

    // Send a payload to the relay server with the udp id
    Result<> sendRelayUdpStage(uint32_t udpId);
    Result<> sendRelaySkipUdpLink();

    // Send a packet to the currently active connection. Throws if disconnected
    Result<> sendPacket(std::shared_ptr<Packet> packet, Protocol protocol = Protocol::Unspecified);

    // Like sendPacket, but forcefully sends via the TCP connection
    Result<> sendPacketTCP(std::shared_ptr<Packet> packet);
    // Like sendPacket, but forcefully sends via the UDP connection
    Result<> sendPacketUDP(std::shared_ptr<Packet> packet);

    // Send a UDP packet to a specific address
    Result<> sendPacketTo(std::shared_ptr<Packet> packet, const NetworkAddress& address);

    Result<> sendRecoveryData(int accountId, uint32_t secretKey);

    void cleanupBox();
    void createBox();

    void togglePacketLogging(bool enabled);
    void toggleForceTcp(bool enabled);

    enum class PollResult {
        None, Tcp, Udp, Both
    };

    Result<PollResult> poll(int timeoutMs);

    Result<bool> poll(Protocol proto, int timeoutMs);

private:
    friend class NetworkManager;

    TcpSocket tcpSocket;
    UdpSocket udpSocket;
    UdpFrameBuffer udpBuffer;

    std::unique_ptr<CryptoBox> cryptoBox;
    util::data::byte* dataBuffer;

    bool dumpPackets = false;
    bool forceUseTcp = false;

    // Write a packet, packet header, and optionally length if the packet is TCP to the given buffer.
    Result<> encodePacket(Packet& packet, ByteBuffer& buffer, bool tcp);

    // Decode a packet from a buffer
    Result<std::shared_ptr<Packet>> decodePacket(ByteBuffer& buffer);

    void dumpPacket(packetid_t id, ByteBuffer& buffer, bool sending);
};
