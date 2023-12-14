#pragma once
#include "socket.hpp"
#include <util/sync.hpp>

class UdpSocket : public Socket {
public:
    using Socket::send;
    UdpSocket();
    ~UdpSocket();

    bool create() override;
    bool connect(const std::string& serverIp, unsigned short port) override;
    int send(const char* data, unsigned int dataSize) override;
    int sendTo(const char* data, unsigned int dataSize, const std::string& address, unsigned short port);
    RecvResult receive(char* buffer, int bufferSize) override;
    bool close() override;
    virtual void disconnect();
    bool poll(int msDelay) override;

    util::sync::AtomicBool connected = false;
protected:
    util::sync::AtomicInt socket_ = 0;

private:
    sockaddr_in destAddr_;
};
