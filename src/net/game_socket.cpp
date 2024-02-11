#include "game_socket.hpp"

#include <data/bytebuffer.hpp>
#include <data/packets/all.hpp>
#include <util/debug.hpp>
#include <util/net.hpp>

constexpr size_t BUF_SIZE = 2 << 17;

using namespace util::data;
using namespace util::debug;

GameSocket::GameSocket() {
    buffer = new byte[BUF_SIZE];
}

GameSocket::~GameSocket() {
    delete[] buffer;
}

std::shared_ptr<Packet> GameSocket::recvPacket(bool onActive) {
    int received;
    if (onActive) {
        ByteBuffer bb;
        bb.grow(4);

        // receive packet size
        tcpSocket.recvExact(reinterpret_cast<char*>(bb.getDataRef().data()), 4);

        auto packetSize = bb.readU32();
        GLOBED_REQUIRE(packetSize < BUF_SIZE, "packet is too big, rejecting")

        tcpSocket.recvExact(reinterpret_cast<char*>(buffer), packetSize);
        received = packetSize;
    } else {
        received = udpSocket.receive(reinterpret_cast<char*>(buffer), BUF_SIZE).result;
    }

    GLOBED_REQUIRE(received > 0, "failed to receive data from a socket")

    GLOBED_REQUIRE(received >= PacketHeader::SIZE, "packet is missing a header")

    ByteBuffer buf(reinterpret_cast<byte*>(buffer), received);

    // read header
    auto header = buf.readValue<PacketHeader>();

    // packet size without the header
    size_t messageLength = received - PacketHeader::SIZE;

#ifdef GLOBED_DEBUG_PACKETS
    PacketLogger::get().record(header.id, header.encrypted, false, received);
#endif

    auto packet = matchPacket(header.id);

    GLOBED_REQUIRE(packet.get() != nullptr, std::string("invalid server-side packet: ") + std::to_string(header.id))

    if (packet->getEncrypted() && !header.encrypted) {
        GLOBED_REQUIRE(false, "server sent a cleartext packet when expected an encrypted one")
    }

    if (header.encrypted) {
        GLOBED_REQUIRE(box.get() != nullptr, "attempted to decrypt a packet when no cryptobox is initialized")
        bytevector& bufvec = buf.getDataRef();

        messageLength = box->decryptInPlace(bufvec.data() + PacketHeader::SIZE, messageLength);
        buf.resize(messageLength + PacketHeader::SIZE);
    }

    try {
        packet->decode(buf);
    } catch (const std::exception& e) {
        auto msg = fmt::format("Decoding packet ID {} failed: {}", header.id, e.what());
        throw std::runtime_error(msg);
    }

    return packet;
}

void GameSocket::sendPacket(std::shared_ptr<Packet> packet) {
    auto buf = this->serializePacket(packet.get());

#ifdef GLOBED_DEBUG_PACKETS
    PacketLogger::get().record(packet->getPacketId(), packet->getEncrypted(), true, buf.size());
#endif

    GLOBED_REQUIRE(
        this->tcpSocket.send(reinterpret_cast<char*>(buf.getDataRef().data()), buf.size()) == buf.size(),
        "failed to send the entire buffer"
    )
}

ByteBuffer GameSocket::serializePacket(Packet* packet) {
    ByteBuffer buf;
    PacketHeader header = {
        .id = packet->getPacketId(),
        .encrypted = packet->getEncrypted()
    };

    buf.writeValue(header);

    packet->encode(buf);

    size_t packetSize = buf.size() - PacketHeader::SIZE;

    if (packet->getEncrypted()) {
        GLOBED_REQUIRE(box.get() != nullptr, "attempted to encrypt a packet when no cryptobox is initialized")
        // grow the vector by CryptoBox::PREFIX_LEN extra bytes to do in-place encryption
        buf.grow(CryptoBox::PREFIX_LEN);
        box->encryptInPlace(buf.getDataRef().data() + PacketHeader::SIZE, packetSize);
    }

    return buf;
}

void GameSocket::sendPacketTo(std::shared_ptr<Packet> packet, const std::string_view address, unsigned short port) {
    auto buf = this->serializePacket(packet.get());

#ifdef GLOBED_DEBUG_PACKETS
    PacketLogger::get().record(packet->getPacketId(), packet->getEncrypted(), true, buf.size());
#endif

    GLOBED_REQUIRE(
        this->udpSocket.sendTo(reinterpret_cast<char*>(buf.getDataRef().data()), buf.size(), address, port) == buf.size(),
        "failed to send the entire buffer"
    )
}

void GameSocket::cleanupBox() {
    box = std::unique_ptr<CryptoBox>(nullptr);
}

void GameSocket::createBox() {
    box = std::make_unique<CryptoBox>();
}

Result<GameSocket::PollResult> GameSocket::pollEither(int msDelay) {
    if (!tcpSocket.connected) {
        GLOBED_UNWRAP_INTO(udpSocket.poll(msDelay), auto res);
        return Ok(res ? PollResult::Udp : PollResult::None);
    }

    GLOBED_SOCKET_POLLFD fds[2];

    fds[0].fd = tcpSocket.socket_;
    fds[0].events = POLLIN;
    fds[1].fd = udpSocket.socket_;
    fds[1].events = POLLIN;

    int result = GLOBED_SOCKET_POLL(fds, 2, msDelay);

    if (result == -1) {
        return Err(util::net::lastErrorString());
    }

    bool tcp = fds[0].revents & POLLIN;
    bool udp = fds[1].revents & POLLIN;

    if (tcp && udp) {
        return Ok(PollResult::Both);
    } else if (tcp) {
        return Ok(PollResult::Tcp);
    } else if (udp) {
        return Ok(PollResult::Udp);
    } else {
        return Ok(PollResult::None);
    }
}
