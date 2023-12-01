#pragma once
#include <defs.hpp>
#include <defs/net.hpp>

class Socket {
public:
    virtual bool create() = 0;
    virtual bool connect(const std::string& serverIp, unsigned short port) = 0;
    virtual int send(const char* data, unsigned int dataSize) = 0;
    int send(const std::string& data);
    virtual int receive(char* buffer, int bufferSize) = 0;
    void receiveExact(char* buffer, int bufferSize);
    virtual bool close();
    virtual ~Socket();
    virtual bool poll(int msDelay) = 0;
};