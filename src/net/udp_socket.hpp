#pragma once
#include "socket.hpp"

#include <defs/platform.hpp>
#include <defs/net.hpp>
#include <util/sync.hpp>

class UdpSocket : public Socket {
public:
    using Socket::send;
    UdpSocket();
    ~UdpSocket();

    Result<> connect(const std::string_view serverIp, unsigned short port) override;
    Result<int> send(const char* data, unsigned int dataSize) override;
    Result<int> sendTo(const char* data, unsigned int dataSize, const std::string_view address, unsigned short port);
    RecvResult receive(char* buffer, int bufferSize) override;
    bool close() override;
    virtual void disconnect();
    Result<bool> poll(int msDelay, bool in = true) override;
    Result<> setNonBlocking(bool nb) override;

    util::sync::AtomicBool connected = false;

#ifdef GLOBED_IS_UNIX
    util::sync::AtomicI32 socket_ = 0;
#else
    util::sync::AtomicU32 socket_ = 0;
#endif

private:
    sockaddr_in destAddr_;
};
