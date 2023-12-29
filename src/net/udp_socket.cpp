#include "udp_socket.hpp"
#include <util/net.hpp>

UdpSocket::UdpSocket() : socket_(0) {
    memset(&destAddr_, 0, sizeof(destAddr_));
}

UdpSocket::~UdpSocket() {
    this->close();
}

bool UdpSocket::create() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    socket_ = sock;
    return sock != -1;
}

bool UdpSocket::connect(const std::string& serverIp, unsigned short port) {
    destAddr_.sin_family = AF_INET;
    destAddr_.sin_port = htons(port);

    bool validIp = (inet_pton(AF_INET, serverIp.c_str(), &destAddr_.sin_addr) > 0);
    // if not a valid IPv4, assume it's a domain and try getaddrinfo
    if (!validIp) {
        auto ip = util::net::getaddrinfo(serverIp);
        validIp = (inet_pton(AF_INET, ip.c_str(), &destAddr_.sin_addr) > 0);
        GLOBED_REQUIRE(validIp, "invalid address was returned as result of getaddrinfo")
    }

    connected = true;
    return true; // No actual connection is established in UDP
}

int UdpSocket::send(const char* data, unsigned int dataSize) {
    GLOBED_REQUIRE(connected, "attempting to call UdpSocket::send on a disconnected socket")

    int retval = sendto(socket_, data, dataSize, 0, reinterpret_cast<struct sockaddr*>(&destAddr_), sizeof(destAddr_));

    if (retval == -1) {
        util::net::throwLastError();
    }

    return retval;
}

int UdpSocket::sendTo(const char* data, unsigned int dataSize, const std::string& address, unsigned short port) {
    // stinky windows returns wsa error 10014 if sockaddr is a stack pointer
    std::unique_ptr<sockaddr_in> addr = std::make_unique<sockaddr_in>();

    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);

    GLOBED_REQUIRE(inet_pton(AF_INET, address.c_str(), &addr->sin_addr) > 0, "tried to connect to an invalid address")

    int retval = sendto(socket_, data, dataSize, 0, reinterpret_cast<struct sockaddr*>(addr.get()), sizeof(sockaddr));

    if (retval == -1) {
        util::net::throwLastError();
    }

    return retval;
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
        fromServer = util::net::sameSockaddr(source, destAddr_);
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

bool UdpSocket::poll(int msDelay) {
    GLOBED_SOCKET_POLLFD fds[1];

    fds[0].fd = socket_;
    fds[0].events = POLLIN;

    int result = GLOBED_SOCKET_POLL(fds, 1, msDelay);

    if (result == -1) {
        util::net::throwLastError();
    }

    return result > 0;
}