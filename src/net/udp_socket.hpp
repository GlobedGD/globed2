#pragma once
#include "socket.hpp"

class UdpSocket : public Socket {
public:
    using Socket::send;
    using Socket::sendAll;
    UdpSocket();

    bool create() override;
    bool connect(const std::string& serverIp, unsigned short port) override;
    int send(const char* data, unsigned int dataSize) override;
    int receive(char* buffer, int bufferSize) override;
    bool close() override;
    virtual void disconnect();
    bool poll(long msDelay) override;

    bool connected = false;
protected:
    int socket_;

private:
    sockaddr_in destAddr_;
};
