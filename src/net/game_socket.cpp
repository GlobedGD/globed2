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
    GLOBED_ASSERT(received >= Packet::HEADER_LEN, "packet is missing a header")

    ByteBuffer buf(reinterpret_cast<byte*>(buffer), received);

    // read header
    packetid_t packetId = buf.read<packetid_t>();
    bool encrypted = buf.readU8() != 0;

    // packet size without the header
    size_t messageLength = received - Packet::HEADER_LEN;

    if (encrypted) {
        bytevector& bufvec = buf.getDataRef();
        messageLength = box.decryptInPlace(bufvec.data(), messageLength);
        buf.resize(messageLength + 3);
    }

    auto packet = matchPacket(packetId);
    if (packet == nullptr) {
        return nullptr;
    }

    packet->decode(buf);

    return packet;
}

void GameSocket::sendPacket(Packet* packet) {
    ByteBuffer buf;
    buf.write<packetid_t>(packet->getPacketId());
    buf.write(static_cast<uint8_t>(packet->getEncrypted()));

    packet->encode(buf);
    size_t packetSize = buf.size() - Packet::HEADER_LEN;

    bytevector& dataref = buf.getDataRef();
    if (packet->getEncrypted()) {
        // grow the vector by CryptoBox::PREFIX_LEN extra bytes to do in-place encryption
        buf.grow(CryptoBox::PREFIX_LEN);
        box.encryptInPlace(dataref.data(), packetSize);
    }

    sendAll(reinterpret_cast<char*>(dataref.data()), dataref.size());
}