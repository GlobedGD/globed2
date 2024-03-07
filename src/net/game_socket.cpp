#include "game_socket.hpp"

#include <data/bytebuffer.hpp>
#include <data/packets/all.hpp>
#include <util/debug.hpp>
#include <util/net.hpp>
#include <util/crypto.hpp>

constexpr size_t BUF_SIZE = 2 << 17;

using namespace util::data;
using namespace util::debug;

GameSocket::GameSocket() {
    buffer = new byte[BUF_SIZE];
}

GameSocket::~GameSocket() {
    delete[] buffer;
}

Result<std::shared_ptr<Packet>> GameSocket::recvPacket(bool onTcpConnection, bool& fromConnected) {
    int received;

    if (onTcpConnection) {
        ByteBuffer bb;
        bb.grow(4);

        // receive the packet length
        GLOBED_UNWRAP(tcpSocket.recvExact(reinterpret_cast<char*>(bb.data().data()), 4));

        auto packetSize = bb.readU32().value_or(0); // must always be 4 bytes so cant error
        GLOBED_REQUIRE_SAFE(packetSize < BUF_SIZE, "packet is too big, rejecting")

        GLOBED_UNWRAP(tcpSocket.recvExact(reinterpret_cast<char*>(buffer), packetSize));
        received = packetSize;
    } else {
        auto recvResult = udpSocket.receive(reinterpret_cast<char*>(buffer), BUF_SIZE);
        received = recvResult.result;
        fromConnected = recvResult.fromServer;
    }

    GLOBED_REQUIRE_SAFE(received >= 0 && received >= PacketHeader::SIZE, "packet is missing a header")
    GLOBED_REQUIRE_SAFE(received < BUF_SIZE, "packet is too large")

    ByteBuffer buf(buffer, received);

    // read header
    auto header = buf.readValue<PacketHeader>().unwrap(); // we know that the header must be present by now.

    // packet size without the header
    size_t messageLength = received - PacketHeader::SIZE;

#ifdef GLOBED_DEBUG_PACKETS
    PacketLogger::get().record(header.id, header.encrypted, false, received);
#endif

    auto packet = matchPacket(header.id);

    GLOBED_REQUIRE_SAFE(packet.get() != nullptr, std::string("invalid server-side packet: ") + std::to_string(header.id))

    if (packet->getEncrypted() && !header.encrypted) {
        GLOBED_REQUIRE_SAFE(false, "server sent a cleartext packet when expected an encrypted one")
    }

    if (header.encrypted) {
        GLOBED_REQUIRE_SAFE(cryptoBox.get() != nullptr, "attempted to decrypt a packet when no cryptobox is initialized")
        bytevector& bufvec = buf.data();

        messageLength = cryptoBox->decryptInPlace(bufvec.data() + PacketHeader::SIZE, messageLength);
        buf.resize(messageLength + PacketHeader::SIZE);
    }

    auto result = packet->decode(buf);;
    if (result.isErr()) {
        return Err(fmt::format("Decoding packet ID {} failed: {}", header.id, result.unwrapErr()));
    }

    return Ok(std::move(packet));
}

Result<std::shared_ptr<Packet>> GameSocket::recvPacket(int timeoutMs, bool& fromConnected, bool& timedOut) {
    timedOut = false;
    fromConnected = false;

    auto pollResult_ = this->poll(timeoutMs);

    GLOBED_UNWRAP_INTO(pollResult_, auto pollResult);

    if (pollResult == PollResult::None) {
        timedOut = true;
        return Err("timed out");
    }

    // prioritize TCP, if the result is Tcp or Both, we care about TCP.
    if (pollResult != PollResult::Udp) {
        fromConnected = true;
        bool _unused;
        return this->recvPacket(true, _unused);
    }

    // else it's a udp packet
    return this->recvPacket(false, fromConnected);
}

Result<> GameSocket::sendPacket(std::shared_ptr<Packet> packet) {
    GLOBED_REQUIRE_SAFE(this->isConnected(), "attempting to send a packet while disconnected")

    ByteBuffer buf;
    GLOBED_UNWRAP(this->serializePacket(packet.get(), buf))

#ifdef GLOBED_DEBUG_PACKETS
    PacketLogger::get().record(packet->getPacketId(), packet->getEncrypted(), true, buf.size());
#endif

    if (packet->getUseTcp()) {
        GLOBED_UNWRAP(tcpSocket.sendAll(reinterpret_cast<const char*>(buf.data().data()), buf.size()));
    } else {
        GLOBED_UNWRAP(udpSocket.send(reinterpret_cast<const char*>(buf.data().data()), buf.size()));
    }

    return Ok();
}

Result<> GameSocket::sendPacketTo(std::shared_ptr<Packet> packet, const std::string_view address, unsigned short port) {
    GLOBED_REQUIRE_SAFE(!packet->getUseTcp(), "cannot send a TCP packet to a UDP connection")

    ByteBuffer buf;
    GLOBED_UNWRAP(this->serializePacket(packet.get(), buf))

#ifdef GLOBED_DEBUG_PACKETS
    PacketLogger::get().record(packet->getPacketId(), packet->getEncrypted(), true, buf.size());
#endif

    GLOBED_UNWRAP_INTO(udpSocket.sendTo(reinterpret_cast<const char*>(buf.data().data()), buf.size(), address, port), auto res)

    GLOBED_REQUIRE_SAFE(
        res == buf.size(),
        "failed to send the entire buffer"
    )

    return Ok();
}

bool GameSocket::isConnected() {
    return tcpSocket.connected;
}

Result<> GameSocket::connect(const std::string_view address, unsigned short port) {
    GLOBED_UNWRAP(tcpSocket.connect(address, port))
    GLOBED_UNWRAP(udpSocket.connect(address, port))

    return Ok();
}

void GameSocket::disconnect() {
    tcpSocket.disconnect();
    udpSocket.disconnect();
}

void GameSocket::cleanupBox() {
    cryptoBox = std::unique_ptr<CryptoBox>(nullptr);
}

void GameSocket::createBox() {
    cryptoBox = std::make_unique<CryptoBox>();
}

Result<> GameSocket::serializePacket(Packet* packet, ByteBuffer& buffer) {
    PacketHeader header = {
        .id = packet->getPacketId(),
        .encrypted = packet->getEncrypted()
    };

    bool tcp = packet->getUseTcp();
    // reserve space for packet length

    size_t startPos = buffer.getPosition();
    if (tcp) {
        buffer.writeU32(0);
    }

    buffer.writeValue<PacketHeader>(header);
    packet->encode(buffer);

    if (packet->getEncrypted()) {
        GLOBED_REQUIRE_SAFE(cryptoBox.get() != nullptr, "attempted to encrypt a packet when no cryptobox is initialized")

        // grow the vector by CryptoBox::PREFIX_LEN extra bytes to do in-place encryption
        buffer.grow(CryptoBox::PREFIX_LEN);
        uint32_t headerSize = PacketHeader::SIZE;
        if (tcp) {
            headerSize += sizeof(uint32_t);
        }

        auto rawSize = buffer.size() - headerSize - startPos - CryptoBox::PREFIX_LEN;
        cryptoBox->encryptInPlace(buffer.data().data() + startPos + headerSize, rawSize);
    }

    if (tcp) {
        size_t lastPos = buffer.getPosition();
        buffer.setPosition(startPos);
        buffer.writeU32(buffer.size() - sizeof(uint32_t) - startPos);
        buffer.setPosition(lastPos);
    }

    return Ok();
}

Result<GameSocket::PollResult> GameSocket::poll(int timeoutMs) {
    if (!tcpSocket.connected) {
        GLOBED_UNWRAP_INTO(udpSocket.poll(timeoutMs), auto res);
        return Ok(res ? PollResult::Udp : PollResult::None);
    }

    GLOBED_SOCKET_POLLFD fds[2];

    fds[0].fd = tcpSocket.socket_;
    fds[0].events = POLLIN;
    fds[1].fd = udpSocket.socket_;
    fds[1].events = POLLIN;

    int result = GLOBED_SOCKET_POLL(fds, 2, timeoutMs);

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
