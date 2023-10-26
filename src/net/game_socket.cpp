#include "game_socket.hpp"
#include <data/bytebuffer.hpp>
#include <data/packets/all.hpp>

using namespace util::data;

GameSocket::GameSocket() {
    buffer = new byte[65536];
}

std::shared_ptr<Packet> GameSocket::recvPacket() {
    auto received = receive(reinterpret_cast<char*>(buffer), 65536);
    GLOBED_ASSERT(received > 0, "failed to receive data from a socket")

    // read the header, 2 bytes for packet ID, 1 byte for encrypted
    GLOBED_ASSERT(received >= 3, "packet is missing a header")

    ByteBuffer header(reinterpret_cast<byte*>(buffer), 3);
    packetid_t packetId = header.read<packetid_t>();
    bool encrypted = header.readU8() != 0;

    byte* messagePtr = buffer + 3;
    size_t messageLength = received - 3;

    if (encrypted) {
        messageLength = box.decryptInPlace(messagePtr, messageLength);
    }

    ByteBuffer buf(messagePtr, messageLength);

    auto packet = matchPacket(packetId);
    if (packet == nullptr) {
        return nullptr;
    }

    packet->decode(buf);

    return packet;
}

void GameSocket::sendPacket(Packet* packet) {
    ByteBuffer buf;
    packet->encode(buf);

    bytevector& data = buf.getDataRef();

    if (!packet->getEncrypted()) {
        // data = box.encrypt(data);
        sendAll(reinterpret_cast<char*>(data.data()), data.size());
        return;
    }

    ByteBuffer encBuf;

    encBuf.write<packetid_t>(packet->getPacketId());
    encBuf.writeU8(static_cast<uint8_t>(packet->getEncrypted()));

    auto encrypted = box.encrypt(data);

    encBuf.writeBytes(encrypted);

    bytevector& finalvec = encBuf.getDataRef();

    sendAll(reinterpret_cast<char*>(finalvec.data()), finalvec.size());
}