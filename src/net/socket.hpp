#pragma once
#include <defs.hpp>
#include <defs/net.hpp>

struct RecvResult {
    bool fromServer; // true if the packet comes from the currently connected server
    int result;
};

class Socket {
public:
    virtual Result<> connect(const std::string_view serverIp, unsigned short port) = 0;
    virtual int send(const char* data, unsigned int dataSize) = 0;
    int send(const std::string_view data);
    virtual RecvResult receive(char* buffer, int bufferSize) = 0;
    virtual bool close();
    virtual ~Socket();
    virtual Result<bool> poll(int msDelay, bool in = true) = 0;
    virtual void setNonBlocking(bool nb) = 0;
};