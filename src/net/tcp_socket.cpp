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

    // attempt a connection
    auto result = ::connect(socket_, reinterpret_cast<struct sockaddr*>(&destAddr_), sizeof(destAddr_));
    GLOBED_REQUIRE_SAFE(result == 0, "tcp socket connection failed")

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

void TcpSocket::recvExact(char* buffer, int bufferSize) {
    unsigned int received = 0;

    do {
        int result = this->receive(buffer + received, bufferSize - received).result;
        if (result <= 0) util::net::throwLastError();
        received += result;
    } while (received < bufferSize);
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

Result<bool> TcpSocket::poll(int msDelay) {
    GLOBED_SOCKET_POLLFD fds[1];

    fds[0].fd = socket_;
    fds[0].events = POLLIN;

    int result = GLOBED_SOCKET_POLL(fds, 1, msDelay);

    if (result == -1) {
        return Err(util::net::lastErrorString());
    }

    return Ok(result > 0);

}
