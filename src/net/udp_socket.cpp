#include "udp_socket.hpp"
#include <defs.hpp>
#include <util/net.hpp>

UdpSocket::UdpSocket() : socket_(0) {
    memset(&destAddr_, 0, sizeof(destAddr_));
}

UdpSocket::~UdpSocket() {
    close();
}

bool UdpSocket::create() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    socket_.store(sock, std::memory_order::relaxed);
    return sock != -1;
}

bool UdpSocket::connect(const std::string& serverIp, unsigned short port) {
    destAddr_.sin_family = AF_INET;
    destAddr_.sin_port = htons(port);

    GLOBED_REQUIRE(inet_pton(AF_INET, serverIp.c_str(), &destAddr_.sin_addr) > 0, "tried to connect to an invalid address");

    connected.store(true, std::memory_order::relaxed);
    return true; // No actual connection is established in UDP
}

int UdpSocket::send(const char* data, unsigned int dataSize) {
    GLOBED_REQUIRE(connected.load(std::memory_order::relaxed), "attempting to call UdpSocket::send on a disconnected socket");
    return sendto(socket_.load(std::memory_order::relaxed), data, dataSize, 0, reinterpret_cast<struct sockaddr*>(&destAddr_), sizeof(destAddr_));
}

void UdpSocket::disconnect() {
    connected.store(false, std::memory_order::relaxed);
}

int UdpSocket::receive(char* buffer, int bufferSize) {
    socklen_t addrLen = sizeof(destAddr_);
    return recvfrom(socket_.load(std::memory_order::relaxed), buffer, bufferSize, 0, reinterpret_cast<struct sockaddr*>(&destAddr_), &addrLen);
}

bool UdpSocket::close() {
    if (!connected.load(std::memory_order::relaxed)) return true;

    connected.store(false, std::memory_order::relaxed);
#ifdef GEODE_IS_WINDOWS
    return ::closesocket(socket_.load(std::memory_order::relaxed)) == 0;
#else
    return ::close(socket_.load(std::memory_order::relaxed)) == 0;
#endif
}

bool UdpSocket::poll(long msDelay) {
    GLOBED_SOCKET_POLLFD fds[1];

    fds[0].fd = socket_.load(std::memory_order::relaxed);
    fds[0].events = POLLIN;

    int result = GLOBED_SOCKET_POLL(fds, 1, (int)msDelay);

    if (result == -1) {
        util::net::throwLastError();
    }

    return result > 0;
}