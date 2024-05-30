#pragma once
#include <defs/minimal_geode.hpp>

class NetworkAddress;

struct RecvResult {
    bool fromServer; // true if the packet comes from the currently connected server
    int result;
};

class Socket {
public:
    virtual Result<> connect(const NetworkAddress& address) = 0;
    virtual Result<int> send(const char* data, unsigned int dataSize) = 0;
    Result<int> send(const std::string_view data);
    virtual RecvResult receive(char* buffer, int bufferSize) = 0;
    virtual bool close();
    virtual ~Socket();
    virtual Result<bool> poll(int msDelay, bool in = true) = 0;
    virtual Result<> setNonBlocking(bool nb) = 0;
};