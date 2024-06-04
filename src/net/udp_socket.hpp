#pragma once
#include "socket.hpp"

#include <defs/platform.hpp>
#include <asp/sync.hpp>

struct sockaddr_in;

class UdpSocket : public Socket {
public:
    using Socket::send;
    UdpSocket();
    ~UdpSocket();

    Result<> connect(const NetworkAddress& address) override;
    Result<int> send(const char* data, unsigned int dataSize) override;
    Result<int> sendTo(const char* data, unsigned int dataSize, const NetworkAddress& address);
    RecvResult receive(char* buffer, int bufferSize) override;
    bool close() override;
    virtual void disconnect();
    Result<bool> poll(int msDelay, bool in = true) override;
    Result<> setNonBlocking(bool nb) override;

    asp::AtomicBool connected = false;

#ifdef GLOBED_IS_UNIX
    asp::AtomicI32 socket_ = 0;
#else
    asp::AtomicSizeT socket_ = 0; // pointer sized
#endif

private:
    std::unique_ptr<sockaddr_in> destAddr_;
};
