#include "udp_socket.hpp"

#include "address.hpp"
#include <defs/assert.hpp>
#include <util/net.hpp>

#ifdef GEODE_IS_WINDOWS
# include <Ws2tcpip.h>
#else
# include <sys/socket.h>
# include <netinet/in.h>
# include <unistd.h>
# include <poll.h>
#endif

UdpSocket::UdpSocket() : socket_(0) {
    destAddr_ = std::make_unique<sockaddr_in>();
    std::memset(destAddr_.get(), 0, sizeof(sockaddr_in));

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    socket_ = sock;

    GLOBED_REQUIRE(sock != -1, "failed to create a udp socket: socket failed");
}

UdpSocket::~UdpSocket() {
    this->close();
}

Result<> UdpSocket::connect(const NetworkAddress& address) {
    destAddr_->sin_family = AF_INET;

    GLOBED_UNWRAP_INTO(address.resolve(), *destAddr_)

    connected = true;
    return Ok();
}

Result<int> UdpSocket::send(const char* data, unsigned int dataSize) {
    GLOBED_REQUIRE_SAFE(connected, "attempting to call UdpSocket::send on a disconnected socket")

    int retval = sendto(socket_, data, dataSize, 0, reinterpret_cast<struct sockaddr*>(destAddr_.get()), sizeof(sockaddr_in));

    if (retval == -1) {
        return Err(util::net::lastErrorString());
    }

    return Ok(retval);
}

Result<int> UdpSocket::sendTo(const char* data, unsigned int dataSize, const NetworkAddress& address) {
    // stinky windows returns wsa error 10014 if sockaddr is a stack pointer
    std::unique_ptr<sockaddr_in> addr = std::make_unique<sockaddr_in>();

    addr->sin_family = AF_INET;

    GLOBED_UNWRAP_INTO(address.resolve(), *addr)

    int retval = sendto(socket_, data, dataSize, 0, reinterpret_cast<struct sockaddr*>(addr.get()), sizeof(sockaddr));

    if (retval == -1) {
        return Err(util::net::lastErrorString());
    }

    return Ok(retval);
}

void UdpSocket::disconnect() {
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

    connected = false;
#ifdef GEODE_IS_WINDOWS
    return ::closesocket(socket_) == 0;
#else
    return ::close(socket_) == 0;
#endif
}

Result<bool> UdpSocket::poll(int msDelay, bool in) {
    GLOBED_SOCKET_POLLFD fds[1];

    fds[0].fd = socket_;
    fds[0].events = in ? POLLIN : POLLOUT;

    int result = GLOBED_SOCKET_POLL(fds, 1, msDelay);

    if (result == -1) {
        return Err(util::net::lastErrorString());
    }

    return Ok(result > 0);
}

Result<> UdpSocket::setNonBlocking(bool nb) {
    GLOBED_UNIMPL("UdpSocket::setNonBlocking")
}