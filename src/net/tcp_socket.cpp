#include "tcp_socket.hpp"
#include <util/net.hpp>

using namespace geode::prelude;


TcpSocket::TcpSocket() : socket_(0) {
    memset(&destAddr_, 0, sizeof(destAddr_));
}

TcpSocket::~TcpSocket() {
    this->close();
}

Result<> TcpSocket::connect(const std::string_view serverIp, unsigned short port) {
    destAddr_.sin_family = AF_INET;
    destAddr_.sin_port = htons(port);

    bool validIp = (inet_pton(AF_INET, serverIp.data(), &destAddr_.sin_addr) > 0);
    // if not a valid IPv4, assume it's a domain and try getaddrinfo
    if (!validIp) {
        auto result = util::net::getaddrinfo(serverIp);
        GLOBED_UNWRAP_INTO(result, auto ip)

        validIp = (inet_pton(AF_INET, ip.c_str(), &destAddr_.sin_addr) > 0);
        GLOBED_REQUIRE_SAFE(validIp, "invalid address was returned by getaddrinfo")
    }

    // create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    socket_ = sock;

    GLOBED_REQUIRE_SAFE(sock != -1, "failed to create a tcp socket: socket failed");

    // attempt a connection with a 5 second timeout
    this->setNonBlocking(true);

    // on a non-blocking socket this always errors with EWOULDBLOCK, ignore the status code
    (void) ::connect(socket_, reinterpret_cast<struct sockaddr*>(&destAddr_), sizeof(destAddr_));

    this->setNonBlocking(false);

    // im crying why does this actually poll for double the length????
    GLOBED_UNWRAP_INTO(this->poll(2500, false), auto pollResult);

    GLOBED_REQUIRE_SAFE(pollResult, "tcp connection timed out, failed to connect after 5 seconds.")

    connected = true;
    return Ok();
}

int TcpSocket::send(const char* data, unsigned int dataSize) {
#ifdef GLOBED_IS_UNIX
    constexpr int flags = MSG_NOSIGNAL;
#else
    constexpr int flags = 0;
#endif

    return ::send(socket_, data, dataSize, flags);
}

Result<> TcpSocket::sendAll(const char* data, unsigned int dataSize) {
    unsigned int totalSent = 0;

    do {
        auto result = this->send(data + totalSent, dataSize - totalSent);
        if (result == -1) {
            return Err(util::net::lastErrorString());
        }

        totalSent += result;
    } while (totalSent < dataSize);

    return Ok();
}

void TcpSocket::disconnect() {
    this->close();
}

RecvResult TcpSocket::receive(char* buffer, int bufferSize) {
    GLOBED_REQUIRE(connected, "attempting to call TcpSocket::receive on a disconnected socket")

    int result = ::recv(socket_, buffer, bufferSize, 0);

    return RecvResult {
        .fromServer = true,
        .result = result,
    };
}

Result<> TcpSocket::recvExact(char* buffer, int bufferSize) {
    GLOBED_REQUIRE_SAFE(connected, "attempting to call TcpSocket::recvExact on a disconnected socket")

    unsigned int received = 0;

    do {
        int result = this->receive(buffer + received, bufferSize - received).result;
        if (result <= 0) return Err(util::net::lastErrorString());
        received += result;
    } while (received < bufferSize);

    return Ok();
}

bool TcpSocket::close() {
    connected = false;

#ifdef GEODE_IS_WINDOWS
    auto res = ::closesocket(socket_) == 0;
#else
    auto res = ::close(socket_) == 0;
#endif

    socket_ = -1;

    return res;
}

Result<bool> TcpSocket::poll(int msDelay, bool in) {
    GLOBED_SOCKET_POLLFD fds[1];

    fds[0].fd = socket_;
    fds[0].events = in ? POLLIN : POLLOUT;

    int result = GLOBED_SOCKET_POLL(fds, 1, msDelay);

    if (result == -1) {
        return Err(util::net::lastErrorString());
    }

    return Ok(result > 0);

}

void TcpSocket::setNonBlocking(bool nb) {
#ifdef GEODE_IS_WINDOWS
    unsigned long mode = nb ? 1 : 0;
    if (SOCKET_ERROR == ioctlsocket(socket_, FIONBIO, &mode)) util::net::throwLastError();
#else
    int flags = fcntl(socket_, F_GETFL);

    if (nb) {
        if (fcntl(socket_, F_SETFL, flags | O_NONBLOCK) < 0) util::net::throwLastError();
    } else {
        if (fcntl(socket_, F_SETFL, flags & (~O_NONBLOCK)) < 0) util::net::throwLastError();
    }
#endif
}
