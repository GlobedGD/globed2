#include "udp_socket.hpp"

#include "address.hpp"
#include <defs/assert.hpp>
#include <managers/settings.hpp>
#include <util/net.hpp>

#ifdef GEODE_IS_WINDOWS
# include <Ws2tcpip.h>
#else
# include <sys/socket.h>
# include <netinet/in.h>
# include <unistd.h>
# include <poll.h>
#endif

UdpSocket::UdpSocket() : socket_(-1) {
    destAddr_ = std::make_unique<sockaddr_in>();
    std::memset(destAddr_.get(), 0, sizeof(sockaddr_in));

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    socket_ = sock;

    globed::netLog("UdpSocket(this={}, fd={}) created", (void*)this, sock);

    GLOBED_REQUIRE(sock != -1, "failed to create a udp socket: socket failed");
}

UdpSocket::~UdpSocket() {
    globed::netLog("UdpSocket(this={}, fd={}) destroyed", (void*)this, socket_.load());

    this->close();
}

UdpSocket& UdpSocket::operator=(UdpSocket&& other) {
    if (this != &other) {
        this->socket_.store(other.socket_.load());
        this->connected.store(other.connected.load());
        this->destAddr_ = std::move(other.destAddr_);

        other.socket_.store(0);
        other.connected.store(false);
    }

    return *this;
}

Result<> UdpSocket::connect(const NetworkAddress& address) {
    if (socket_ == -1) {
        return Err("This UDP socket has already been closed and cannot be reused");
    }

    globed::netLog("UdpSocket::connect(this={}, address={})", (void*)this, GLOBED_LAZY(address.toString()));

    destAddr_->sin_family = AF_INET;

    GLOBED_UNWRAP_INTO(address.resolve(), *destAddr_)

    connected = true;
    return Ok();
}

Result<int> UdpSocket::send(const char* data, unsigned int dataSize) {
    GLOBED_REQUIRE_SAFE(connected, "attempting to call UdpSocket::send on a disconnected socket")

    globed::netLog("UdpSocket::send(this={}, data={}, size={})", (void*)this, (void*)data, dataSize);

    int retval = sendto(socket_, data, dataSize, 0, reinterpret_cast<struct sockaddr*>(destAddr_.get()), sizeof(sockaddr_in));

    if (retval == -1) {
        auto errmsg = util::net::lastErrorString();
        globed::netLog("UdpSocket::send failed, code {}: {}", retval, errmsg);
        return Err(fmt::format("sendto failed ({}): {}", retval, errmsg));
    }

    return Ok(retval);
}

Result<int> UdpSocket::sendTo(const char* data, unsigned int dataSize, const NetworkAddress& address) {
    globed::netLog("UdpSocket::sendTo(this={}, data={}, size={}, address={})", (void*)this, (void*)data, dataSize, GLOBED_LAZY(address.toString()));

    // stinky windows returns wsa error 10014 if sockaddr is a stack pointer
    std::unique_ptr<sockaddr_in> addr = std::make_unique<sockaddr_in>();

    addr->sin_family = AF_INET;

    GLOBED_UNWRAP_INTO(address.resolve(), *addr)

    int retval = sendto(socket_, data, dataSize, 0, reinterpret_cast<struct sockaddr*>(addr.get()), sizeof(sockaddr));

    if (retval == -1) {
        auto errmsg = util::net::lastErrorString();
        globed::netLog("UdpSocket::sendTo failed, code {}: {}", retval, errmsg);
        return Err(fmt::format("sendto failed ({}): {}", retval, errmsg));
    }

    return Ok(retval);
}

void UdpSocket::disconnect() {
    globed::netLog("UdpSocket::disconnect(this={})", (void*)this);
    connected = false;
}

RecvResult UdpSocket::receive(char* buffer, int bufferSize) {
    sockaddr_in source;
    socklen_t addrLen = sizeof(source);

    int result = recvfrom(socket_, buffer, bufferSize, 0, reinterpret_cast<struct sockaddr*>(&source), &addrLen);

    bool fromServer = false;
    if (this->connected) {
        fromServer = util::net::sameSockaddr(source, *destAddr_);
    }

    return RecvResult {
        .fromServer = fromServer,
        .result = result,
    };
}

bool UdpSocket::close() {
    if (!connected) return true;

    globed::netLog("UdpSocket::close(this={}, fd={})", (void*)this, socket_.load());

    connected = false;
#ifdef GEODE_IS_WINDOWS
    bool res = ::closesocket(socket_) == 0;
#else
    bool res =  ::close(socket_) == 0;
#endif
    socket_ = -1;

    return res;
}

Result<bool> UdpSocket::poll(int msDelay, bool in) {
    GLOBED_SOCKET_POLLFD fds[1];

    fds[0].fd = socket_;
    fds[0].events = in ? POLLIN : POLLOUT;

    int result = GLOBED_SOCKET_POLL(fds, 1, msDelay);

    if (result == -1) {
        return Err(fmt::format("udp poll failed ({}): {}", result, util::net::lastErrorString()));
    }

    return Ok(result > 0);
}

Result<> UdpSocket::setNonBlocking(bool nb) {
    GLOBED_UNIMPL("UdpSocket::setNonBlocking")
}