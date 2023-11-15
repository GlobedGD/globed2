#include "game_socket.hpp"
#include <data/bytebuffer.hpp>
#include <data/packets/all.hpp>
#include <util/debugging.hpp>

const size_t BUF_SIZE = 65536;

using namespace util::data;
using namespace util::debugging;

GameSocket::GameSocket() {
    buffer = new byte[BUF_SIZE];
}

GameSocket::~GameSocket() {
    delete[] buffer;
}

std::shared_ptr<Packet> GameSocket::recvPacket() {
    auto received = receive(reinterpret_cast<char*>(buffer), BUF_SIZE);
    GLOBED_ASSERT(received > 0, "failed to receive data from a socket")

    // read the header, 2 bytes for packet ID, 1 byte for encrypted
    GLOBED_ASSERT(received >= Packet::HEADER_LEN, "packet is missing a header")

    ByteBuffer buf(reinterpret_cast<byte*>(buffer), received);

    // read header
    packetid_t packetId = buf.readU16();
    bool encrypted = buf.readU8() != 0;

    // packet size without the header
    size_t messageLength = received - Packet::HEADER_LEN;

#ifdef GLOBED_DEBUG_PACKETS
    PacketLogger::get().record(packetId, encrypted, false, received);
#endif

    auto packet = matchPacket(packetId);

    GLOBED_ASSERT(packet.get() != nullptr, std::string("invalid server-side packet: ") + std::to_string(packetId))

    if (packet->getEncrypted() && !encrypted) {
        GLOBED_ASSERT(false, "server sent a cleartext packet when expected an encrypted one")
    }

    if (encrypted) {
        GLOBED_ASSERT(box.get() != nullptr, "attempted to decrypt a packet when no cryptobox is initialized")
        bytevector& bufvec = buf.getDataRef();

        messageLength = box->decryptInPlace(bufvec.data() + Packet::HEADER_LEN, messageLength);
        buf.resize(messageLength + Packet::HEADER_LEN);
    }

    packet->decode(buf);

    return packet;
}

void GameSocket::sendPacket(std::shared_ptr<Packet> packet) {
    ByteBuffer buf;
    buf.writeU16(packet->getPacketId());
    buf.writeU8(static_cast<uint8_t>(packet->getEncrypted()));

    packet->encode(buf);
    size_t packetSize = buf.size() - Packet::HEADER_LEN;

    bytevector& dataref = buf.getDataRef();
    if (packet->getEncrypted()) {
        GLOBED_ASSERT(box.get() != nullptr, "attempted to encrypt a packet when no cryptobox is initialized")
        // grow the vector by CryptoBox::PREFIX_LEN extra bytes to do in-place encryption
        buf.grow(CryptoBox::PREFIX_LEN);
        box->encryptInPlace(dataref.data() + Packet::HEADER_LEN, packetSize);
    }

#ifdef GLOBED_DEBUG_PACKETS
    PacketLogger::get().record(packet->getPacketId(), packet->getEncrypted(), true, dataref.size());
#endif

    sendAll(reinterpret_cast<char*>(dataref.data()), dataref.size());
}

void GameSocket::cleanupBox() {
    box = std::unique_ptr<CryptoBox>(nullptr);
}

void GameSocket::createBox() {
    box = std::make_unique<CryptoBox>();
}