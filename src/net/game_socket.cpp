#include "game_socket.hpp"

#include <data/bytebuffer.hpp>
#include <data/packets/match.hpp>
#include <managers/settings.hpp>
#include <util/debug.hpp>
#include <util/net.hpp>
#include <util/format.hpp>
#include <util/crypto.hpp>

#ifdef GEODE_IS_WINDOWS
# include <WinSock2.h>
#else
# include <sys/socket.h>
# include <poll.h>
#endif

constexpr size_t DATA_BUF_SIZE = 2 << 18;
constexpr uint32_t RELAY_MAGIC = 0x7f8a9b0c;
constexpr uint32_t RELAY_MAGIC_SKIP_LINK = 0x7f8a9b0d;

using namespace util::data;
using namespace util::debug;
using namespace asp::time;
using PollResult = GameSocket::PollResult;
using Protocol = GameSocket::Protocol;
using ReceivedPacket = GameSocket::ReceivedPacket;

GameSocket::GameSocket() {
    globed::netLog("GameSocket: created new socket with bufsize={}", DATA_BUF_SIZE);
    dataBuffer = new byte[DATA_BUF_SIZE];
}

GameSocket::~GameSocket() {
    globed::netLog("GameSocket: destroying socket");
    this->disconnect();
    delete[] dataBuffer;
}

Result<> GameSocket::connect(const NetworkAddress& address, bool isRecovering) {
#ifdef GLOBED_DEBUG
    auto r = address.resolveToString();
    std::string resolved = "<unresolved>";
    if (r) {
        resolved = r.unwrap();
    } else {
        resolved = fmt::format("<unresolved>: {}", r.unwrapErr());
    }

    log::debug("Connecting to {} (resolved to {})", address.toString(), resolved);
#endif

    GLOBED_UNWRAP(tcpSocket.connect(address))
    GLOBED_UNWRAP(udpSocket.connect(address))

    globed::netLog("GameSocket::connect sending magic byte (recovery = {})", isRecovering);

    // send a magic byte telling the server whether we are recovering or not
    uint8_t byte = isRecovering ? MARKER_CONN_RECOVERY : MARKER_CONN_INITIAL;
    GLOBED_UNWRAP(tcpSocket.send(reinterpret_cast<const char*>(&byte), 1));

    return Ok();
}

Result<> GameSocket::connectWithRelay(const NetworkAddress& address, const NetworkAddress& relayAddress, bool isRecovering) {
    GLOBED_UNWRAP(this->connect(relayAddress, isRecovering));

    auto resolvedHost = GEODE_UNWRAP(address.resolveToString());

    ByteBuffer buffer;
    buffer.writeU32(RELAY_MAGIC); // magic
    buffer.writeU32(resolvedHost.size());
    buffer.writeBytes(resolvedHost.data(), resolvedHost.size());

    GLOBED_UNWRAP(tcpSocket.send((const char*) buffer.data().data(), buffer.size()));

    return Ok();
}

void GameSocket::disconnect() {
    globed::netLog("GameSocket::disconnect");

    tcpSocket.disconnect();
    udpSocket.disconnect();
    udpBuffer.clear();
}

bool GameSocket::isConnected() {
    return tcpSocket.connected;
}

Result<std::shared_ptr<Packet>> GameSocket::recvPacketTCP() {
    ByteBuffer bb;
    bb.grow(4);

    // receive the packet length
    GLOBED_UNWRAP(tcpSocket.recvExact(reinterpret_cast<char*>(bb.data().data()), 4));

    auto packetSize = bb.readU32().unwrapOr(0); // must always be 4 bytes so cant error
    globed::netLog("GameSocket::recvPacketTCP received message length, {} bytes", packetSize);

    GLOBED_REQUIRE_SAFE(packetSize < DATA_BUF_SIZE, "packet is too big, rejecting")

    GLOBED_UNWRAP(tcpSocket.recvExact(reinterpret_cast<char*>(dataBuffer), packetSize));

    globed::netLog("GameSocket::recvPacketTCP received entirety of the message, decoding!");

    ByteBuffer buf(dataBuffer, packetSize);

    auto retval = this->decodePacket(buf);
    if (retval) {
        auto& pkt = retval.unwrap();
        PacketLogger::get().record(pkt->getPacketId(), pkt->getEncrypted(), false, buf.size());
    }

    return retval;
}

Result<std::optional<ReceivedPacket>> GameSocket::recvPacketUDP(bool skipMarker) {
    auto recvResult = udpSocket.receive(reinterpret_cast<char*>(dataBuffer), DATA_BUF_SIZE);

    ReceivedPacket out;
    out.fromConnected = recvResult.fromServer;

    if (recvResult.result < 0) {
        auto code = util::net::lastErrorCode();

        // So ios likes to be quirky, and if you close gd and leave it in the background for some time,
        // after coming back it will kill the udp socket and return ENOTCONN.
        // we don't have much choice but to just recreate the socket here.
        // i made this GLOBED_IS_UNIX because in theory this may happen on android at some point too (?) although i have never seen it
#ifdef GLOBED_IS_UNIX
        if (code == ENOTCONN) {
            globed::netLog("GameSocket::recvPacketUDP - recreating UDP socket that was killed by the OS..");
            this->disconnect();
            udpSocket = UdpSocket{};
            return Err("socket was destroyed");
        }
#endif

        globed::netLog("GameSocket::recvPacketUDP fail: code {}", recvResult.result);
        return Err(fmt::format("udp recv failed ({}): {}", recvResult.result, util::net::lastErrorString(code)));
    }

    globed::netLog("GameSocket::recvPacketUDP received {} bytes (fromConnected = {})", (size_t)recvResult.result, out.fromConnected);

    ByteBuffer buf(dataBuffer, (size_t)recvResult.result);

    // if not from active server, dont't read the marker
    if (!out.fromConnected || skipMarker) {
        GLOBED_UNWRAP_INTO(this->decodePacket(buf), out.packet);

        if (out.packet) {
            PacketLogger::get().record(out.packet->getPacketId(), out.packet->getEncrypted(), false, buf.size());
        }

        return Ok(std::move(out));
    }

    // check if it is a full packet or a frame,
    auto marker = buf.readU8();

    if (marker.isErr()) {
        return Err(fmt::to_string(marker.unwrapErr()));
    }

    globed::netLog("GameSocket::recvPacketUDP marker = {:X}", (int) *marker);

    if (*marker == MARKER_UDP_PACKET) {
        GLOBED_UNWRAP_INTO(this->decodePacket(buf), out.packet);
    } else if (*marker == MARKER_UDP_FRAME) {
        GLOBED_UNWRAP_INTO(udpBuffer.pushFrameFromBuffer(buf), auto maybeBuf);
        if (!maybeBuf.empty()) {
            ByteBuffer toDecode(std::move(maybeBuf));
            GLOBED_UNWRAP_INTO(this->decodePacket(toDecode), out.packet);
        } else {
            return Ok(std::nullopt);
        }
    } else {
        return Err("invalid marker at the start of a udp packet");
    }

    if (out.packet) {
        PacketLogger::get().record(out.packet->getPacketId(), out.packet->getEncrypted(), false, buf.size());
    }

    return Ok(std::move(out));
}

Result<ReceivedPacket> GameSocket::recvPacket(int timeoutMs) {
    // negative value means poll indefinitely until either tcp or udp receives data
    GLOBED_UNWRAP_INTO(this->poll(timeoutMs), auto pollResult);

    if (pollResult == PollResult::None) {
        return Err("timed out");
    }

    globed::netLog("GameSocket::recvPacket successful poll on {}, trying to receive", (int) pollResult);

    // prioritize TCP, if the result is Tcp or Both, we care about TCP.
    if (pollResult != PollResult::Udp) {
        // It is possible that the tcp socket was disconnected in the middle of the poll,
        // in which case we probably should not try and receive data from it
        if (!tcpSocket.connected) {
            if (pollResult == PollResult::Tcp) {
                return Err("socket was abruptly disconnected");
            } else {
                pollResult = PollResult::Udp; // Both -> Udp
            }
        } else {
            auto res = this->recvPacketTCP();

            if (res) {
                auto pkt = std::move(res).unwrap();

                globed::netLog("GameSocket::recvPacket returning received TCP packet (id = {})", pkt ? pkt->getPacketId() : 0);

                return Ok(ReceivedPacket {
                    .packet = std::move(pkt),
                    .fromConnected = true
                });
            } else {
                globed::netLog("GameSocket::recvPacket error receiving TCP packet: {}", res.unwrapErr());
                return Err(fmt::format("recvPacketTCP failed: {}", res.unwrapErr()));
            }
        }
    }

    // else it's a udp packet
    auto udpres = this->recvPacketUDP();

    if (udpres && udpres.unwrap().has_value()) {
        auto pkt = std::move(**udpres);
        globed::netLog(
            "GameSocket::recvPacket returning received UDP packet (id = {}, fromConnected = {})",
            pkt.packet ? pkt.packet->getPacketId() : 0, pkt.fromConnected
        );
        return Ok(std::move(pkt));
    } else if (!udpres) {
        globed::netLog("GameSocket::recvPacket error receiving UDP packet: {}", udpres.unwrapErr());
        return Err("recvPacketUDP failed: {}", udpres.unwrapErr());
    }

    // if it was a frame keep trying
    globed::netLog("GameSocket::recvPacket UDP frame was received, waiting for other frames..");

    for (;;) {
        GLOBED_UNWRAP_INTO(udpSocket.poll(25), auto pollres);
        if (!pollres) {
            globed::netLog("GameSocket::recvPacket timed out waiting for other udp frames!");
            return Err("timed out");
        }

        auto udpres = this->recvPacketUDP();
        if (udpres && udpres.unwrap().has_value()) {
            auto pkt = std::move(**udpres);

            globed::netLog(
                "GameSocket::recvPacket returning received UDP (frame!) packet (id = {}, fromConnected = {})",
                pkt.packet ? pkt.packet->getPacketId() : 0, pkt.fromConnected
            );

            return Ok(std::move(pkt));
        } else if (!udpres) {
            globed::netLog("GameSocket::recvPacket error receiving (frame!) UDP packet: {}", udpres.unwrapErr());
            return Err(fmt::format("recvPacketUDP failed: {}", std::move(std::move(udpres).unwrapErr())));
        } else {
            globed::netLog("GameSocket::recvPacket received UDP frame but message is incomplete, looping..");
        }
    }
}

Result<std::vector<uint8_t>> GameSocket::recvRawTcpData(size_t size, int timeoutMs) {
    GLOBED_UNWRAP_INTO(this->poll(Protocol::Tcp, timeoutMs), auto pollResult);

    if (!pollResult) {
        return Err("timed out");
    }

    if (size == 0) {
        uint32_t packetSize;
        GLOBED_UNWRAP(tcpSocket.recvExact((char*)&packetSize, sizeof(uint32_t)));
        size = asp::data::byteswap(packetSize);
    }

    if (size >= 2 << 26) {
        // 64mb is too much sorry buddy
        return Err("server sent a packet that is too large to accept: {}", size);
    }

    std::vector<uint8_t> out(size);
    GLOBED_UNWRAP(tcpSocket.recvExact((char*) out.data(), size));

    return Ok(std::move(out));
}

Result<> GameSocket::sendRelayUdpStage(uint32_t udpId) {
    ByteBuffer buf;
    buf.writeU32(RELAY_MAGIC);
    buf.writeU32(udpId);

    GLOBED_UNWRAP(udpSocket.send((const char*) buf.data().data(), buf.size()));

    return Ok();
}

Result<> GameSocket::sendRelaySkipUdpLink() {
    ByteBuffer buf;
    buf.writeU32(RELAY_MAGIC_SKIP_LINK);

    GLOBED_UNWRAP(tcpSocket.send((const char*) buf.data().data(), buf.size()));

    return Ok();
}

Result<ReceivedPacket> GameSocket::recvPacket() {
    return this->recvPacket(-1);
}

Result<> GameSocket::sendPacket(std::shared_ptr<Packet> packet, Protocol protocol) {
    GLOBED_REQUIRE_SAFE(this->isConnected(), "attempting to send a packet while disconnected")

    globed::netLog("GameSocket::sendPacket(packet={{id={}, encrypted={}}}, protocol={})", packet->getPacketId(), packet->getEncrypted(), (int) protocol);

    bool useTcp = false;

    if (forceUseTcp) {
        useTcp = true;
    } else switch (protocol) {
        case Protocol::Tcp: useTcp = true; break;
        case Protocol::Udp: useTcp = false; break;
        default: useTcp = packet->getUseTcp(); break;
    }

    ByteBuffer buf;
    GLOBED_UNWRAP(this->encodePacket(*packet, buf, useTcp))

    if (dumpPackets) {
        this->dumpPacket(packet->getPacketId(), buf, true);
    }

#ifdef GLOBED_DEBUG_PACKETS
    PacketLogger::get().record(packet->getPacketId(), packet->getEncrypted(), true, buf.size());
#endif

    if (useTcp) {
        GLOBED_UNWRAP(tcpSocket.sendAll(reinterpret_cast<const char*>(buf.data().data()), buf.size()));
    } else {
        GLOBED_UNWRAP(udpSocket.send(reinterpret_cast<const char*>(buf.data().data()), buf.size()));
    }

    return Ok();
}

Result<> GameSocket::sendPacketTCP(std::shared_ptr<Packet> packet) {
    return this->sendPacket(packet, Protocol::Tcp);
}

Result<> GameSocket::sendPacketUDP(std::shared_ptr<Packet> packet) {
    return this->sendPacket(packet, Protocol::Udp);
}

Result<> GameSocket::sendPacketTo(std::shared_ptr<Packet> packet, const NetworkAddress& address) {
    globed::netLog("GameSocket::sendPacketTo(packet={{id={}, encrypted={}}}, address={})", packet->getPacketId(), packet->getEncrypted(), address.toString());

    GLOBED_REQUIRE_SAFE(!packet->getUseTcp(), "cannot send a TCP packet to a UDP connection")

    ByteBuffer buf;
    GLOBED_UNWRAP(this->encodePacket(*packet, buf, false))

    if (dumpPackets) {
        this->dumpPacket(packet->getPacketId(), buf, true);
    }

#ifdef GLOBED_DEBUG_PACKETS
    PacketLogger::get().record(packet->getPacketId(), packet->getEncrypted(), true, buf.size());
#endif

    GLOBED_UNWRAP_INTO(udpSocket.sendTo(reinterpret_cast<const char*>(buf.data().data()), buf.size(), address), auto res)

    GLOBED_REQUIRE_SAFE(
        res == buf.size(),
        "failed to send the entire buffer"
    )

    return Ok();
}

Result<> GameSocket::sendRecoveryData(int accountId, uint32_t secretKey) {
    globed::netLog("GameSocket::sendRecoveryData(accountId={}, secretKey={})", accountId, secretKey);

    ByteBuffer bb;
    bb.writeI32(accountId);
    bb.writeU32(secretKey);

    return tcpSocket.sendAll(reinterpret_cast<const char*>(bb.data().data()), bb.size());
}

void GameSocket::cleanupBox() {
    globed::netLog("GameSocket::cleanupBox");
    cryptoBox = std::unique_ptr<CryptoBox>(nullptr);
}

void GameSocket::createBox() {
    globed::netLog("GameSocket::createBox");
    cryptoBox = std::make_unique<CryptoBox>();
}

void GameSocket::togglePacketLogging(bool state) {
    dumpPackets = state;
}

void GameSocket::toggleForceTcp(bool enabled) {
    forceUseTcp = enabled;
}

Result<PollResult> GameSocket::poll(int timeoutMs) {
    globed::netLog("GameSocket::poll(timeoutMs={})", timeoutMs);

    if (!tcpSocket.connected) {
        GLOBED_UNWRAP_INTO(udpSocket.poll(timeoutMs), auto res);
        globed::netLog("GameSocket::poll polled only udp, result: {}", res);
        return Ok(res ? PollResult::Udp : PollResult::None);
    }

    GLOBED_SOCKET_POLLFD fds[2];

    fds[0].fd = tcpSocket.socket_;
    fds[0].events = POLLIN;
    fds[1].fd = udpSocket.socket_;
    fds[1].events = POLLIN;

    int result = GLOBED_SOCKET_POLL(fds, 2, timeoutMs);

    if (result == -1) {
        auto code = util::net::lastErrorCode();
        globed::netLog("(E) GameSocket::poll failed! code {}", code);
        return Err(util::net::lastErrorString(code));
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

Result<bool> GameSocket::poll(Protocol proto, int timeoutMs) {
    globed::netLog("GameSocket::poll(proto={}, timeoutMs={})", (int) proto, timeoutMs);

    GLOBED_REQUIRE_SAFE(proto != Protocol::Unspecified, "invalid protocol");

    GLOBED_SOCKET_POLLFD fd;
    fd.events = POLLIN;

    if (proto == Protocol::Tcp) {
        if (!tcpSocket.connected) {
            return Err("TCP socket is not connected");
        }

        fd.fd = tcpSocket.socket_;
    } else {
        fd.fd = udpSocket.socket_;
    }

    int result = GLOBED_SOCKET_POLL(&fd, 1, timeoutMs);
    if (result == -1) {
        auto code = util::net::lastErrorCode();
        globed::netLog("(E) GameSocket::poll failed! code {}", code);
        return Err(util::net::lastErrorString(code));
    }

    bool error = (fd.revents & POLLERR) || (fd.revents & POLLHUP) || (fd.revents & POLLNVAL);
    if (error) {
        globed::netLog("(E) GameSocket::poll failed (revents = {})", fd.revents);
        return Err("game socket poll failed");
    }

    globed::netLog("GameSocket::poll result: {}", bool(fd.revents & POLLIN));

    return Ok((bool) (fd.revents & POLLIN));
}

Result<> GameSocket::encodePacket(Packet& packet, ByteBuffer& buffer, bool tcp) {
    PacketHeader header = {
        .id = packet.getPacketId(),
        .encrypted = packet.getEncrypted(),
    };

    // reserve space for packet length when using TCP
    size_t startPos = buffer.getPosition();

    if (tcp) {
        buffer.writeU32(0);
    }

    buffer.writeValue<PacketHeader>(header);
    packet.encode(buffer);

    globed::netLog("GameSocket::encodePacket: encoding packet: id={}, encrypted={}, totalsize={} (pre-encryption)", header.id, header.encrypted, buffer.size());

    if (packet.getEncrypted()) {
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

    // write length
    if (tcp) {
        size_t lastPos = buffer.getPosition();
        size_t packetSize = buffer.size() - sizeof(uint32_t) - startPos;

        buffer.setPosition(startPos);
        buffer.writeU32(packetSize);
        buffer.setPosition(lastPos);
    }

    return Ok();
}

Result<std::shared_ptr<Packet>> GameSocket::decodePacket(ByteBuffer& buffer) {
    // read header
    auto header = buffer.readValue<PacketHeader>().unwrap(); // we know that the header must be present by now.

    // packet size without the header
    size_t messageStart = buffer.getPosition();
    size_t messageLength = buffer.size() - messageStart;

    globed::netLog("GameSocket::decodePacket: Decoded header: id={}, encrypted={}, length={}", header.id, header.encrypted, messageLength);

    auto packet = matchPacket(header.id);

    GLOBED_REQUIRE_SAFE(packet.get() != nullptr, std::string("invalid server-side packet: ") + std::to_string(header.id))

    if (packet->getEncrypted() && !header.encrypted) {
        globed::netLog("(W) GameSocket::decodePacket: warning, mismatched encryption!!");
        GLOBED_REQUIRE_SAFE(false, fmt::format("server sent a cleartext packet when expected an encrypted one ({})", header.id))
    }

    if (header.encrypted) {
        GLOBED_REQUIRE_SAFE(cryptoBox.get() != nullptr, "attempted to decrypt a packet when no cryptobox is initialized")
        bytevector& bufvec = buffer.data();

        GLOBED_UNWRAP_INTO(cryptoBox->decryptInPlace(bufvec.data() + messageStart, messageLength), messageLength);
        buffer.resize(messageStart + messageLength);
    }

    if (dumpPackets) {
        this->dumpPacket(header.id, buffer, false);
    }

    auto result = packet->decode(buffer);
    if (result.isErr()) {
        auto errmsg = ByteBuffer::strerror(result.unwrapErr());

        globed::netLog("(W) GameSocket::decodePacket: failed to decode packet: {}", errmsg);

        return Err(fmt::format("Decoding packet ID {} failed: {}", header.id, errmsg));
    }

    return Ok(std::move(packet));
}

void GameSocket::dumpPacket(packetid_t id, ByteBuffer& buffer, bool sending) {
    log::debug("{} packet {}", sending ? "Sending" : "Receiving", id);

    auto folder = Mod::get()->getSaveDir() / "packets";
    (void) geode::utils::file::createDirectoryAll(folder);
    util::misc::callOnce("networkmanager-log-to-file", [&] {
        log::debug("Packet log folder: {}", folder);
    });

    auto datetime = util::format::formatDateTime(SystemTime::now());
    auto filepath = folder / fmt::format("{}-{}.bin", id, datetime);

    std::ofstream fs(filepath, std::ios::binary);

    const auto& vec = buffer.data();
    fs.write(reinterpret_cast<const char*>(vec.data()), vec.size());
}