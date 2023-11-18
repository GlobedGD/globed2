#pragma once
#include "socket.hpp"
#include <util/sync.hpp>

class UdpSocket : public Socket {
public:
    using Socket::send;
    using Socket::sendAll;
    UdpSocket();
    ~UdpSocket();

    bool create() override;
    bool connect(const std::string& serverIp, unsigned short port) override;
    int send(const char* data, unsigned int dataSize) override;
    int receive(char* buffer, int bufferSize) override;
    bool close() override;
    virtual void disconnect();
    bool poll(long msDelay) override;

    util::sync::AtomicBool connected = false;
protected:
    util::sync::AtomicInt socket_ = 0;

private:
    sockaddr_in destAddr_;
};
