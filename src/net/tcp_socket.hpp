#pragma once
#include "socket.hpp"

#include <defs/platform.hpp>
#include <defs/net.hpp>
#include <util/sync.hpp>

class TcpSocket : public Socket {
public:
    using Socket::send;
    TcpSocket();
    ~TcpSocket();

    Result<> connect(const std::string_view serverIp, unsigned short port) override;
    Result<int> send(const char* data, unsigned int dataSize) override;
    Result<> sendAll(const char* data, unsigned int dataSize);
    RecvResult receive(char* buffer, int bufferSize) override;
    Result<> recvExact(char* buffer, int bufferSize);

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
