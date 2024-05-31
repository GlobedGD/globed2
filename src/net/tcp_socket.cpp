#include "tcp_socket.hpp"

#include "address.hpp"
#include <util/net.hpp>

#ifdef GEODE_IS_WINDOWS
# include <WinSock2.h>
#else
# include <netinet/in.h>
# include <fcntl.h>
# include <poll.h>
# include <unistd.h>
#endif

using namespace geode::prelude;

TcpSocket::TcpSocket() : socket_(0) {
    destAddr_ = std::make_unique<sockaddr_in>();
    std::memset(destAddr_.get(), 0, sizeof(sockaddr_in));
}

TcpSocket::~TcpSocket() {
    this->close();
}

Result<> TcpSocket::connect(const NetworkAddress& address) {
    destAddr_->sin_family = AF_INET;

    GLOBED_UNWRAP_INTO(address.resolve(), *destAddr_)

    // create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    socket_ = sock;

    GLOBED_REQUIRE_SAFE(sock != -1, "failed to create a tcp socket: socket failed");

    // attempt a connection with a 5 second timeout
    GLOBED_UNWRAP(this->setNonBlocking(true));

    // on a non-blocking socket this always errors with EWOULDBLOCK, ignore the status code
    (void) ::connect(socket_, reinterpret_cast<struct sockaddr*>(destAddr_.get()), sizeof(sockaddr_in));

    GLOBED_UNWRAP(this->setNonBlocking(false));

    // im crying why does this actually poll for double the length????
    GLOBED_UNWRAP_INTO(this->poll(2500, false), auto pollResult);

    GLOBED_REQUIRE_SAFE(pollResult, "connection timed out, failed to connect after 5 seconds.")

    connected = true;
    return Ok();
}

Result<int> TcpSocket::send(const char* data, unsigned int dataSize) {
#ifdef GLOBED_IS_UNIX
    constexpr int flags = MSG_NOSIGNAL;
#else
    constexpr int flags = 0;
#endif

    auto result = ::send(socket_, data, dataSize, flags);
    if (result == -1) {
        this->maybeDisconnect();
        return Err(util::net::lastErrorString());
    }

    return Ok(result);
}

Result<> TcpSocket::sendAll(const char* data, unsigned int dataSize) {
    unsigned int totalSent = 0;

    do {
        auto result_ = this->send(data + totalSent, dataSize - totalSent);
        GLOBED_UNWRAP_INTO(result_, auto result);

        totalSent += result;
    } while (totalSent < dataSize);

    return Ok();
}

void TcpSocket::disconnect() {
    this->close();
}

RecvResult TcpSocket::receive(char* buffer, int bufferSize) {
    if (!connected) {
        return RecvResult {
            .fromServer = true,
            .result = -1
        };
    }

    int result = ::recv(socket_, buffer, bufferSize, 0);
    if (result == -1) {
        this->maybeDisconnect();
    } else if (result == 0) {
        this->disconnect();
    }

    return RecvResult {
        .fromServer = true,
        .result = result,
    };
}

Result<> TcpSocket::recvExact(char* buffer, int bufferSize) {
    GLOBED_REQUIRE_SAFE(connected, "attempting to call TcpSocket::recvExact on a disconnected socket")

    unsigned int received = 0;

    do {
        int result = this->receive(buffer + received, bufferSize - received).result;
        if (result < 0) return Err(util::net::lastErrorString());
        if (result == 0) return Err("connection was closed by the server");
        received += result;
    } while (received < bufferSize);

    return Ok();
}

bool TcpSocket::close() {
    connected = false;

#ifdef GEODE_IS_WINDOWS
    auto res = ::closesocket(socket_) == 0;
#else
    auto res = ::close(socket_) == 0;
#endif

    socket_ = -1;

    return res;
}

Result<bool> TcpSocket::poll(int msDelay, bool in) {
    GLOBED_SOCKET_POLLFD fds[1];

    fds[0].fd = socket_;
    fds[0].events = in ? POLLIN : POLLOUT;

    int result = GLOBED_SOCKET_POLL(fds, 1, msDelay);

    if (result == -1) {
        return Err(util::net::lastErrorString());
    }

    return Ok(result > 0);

}

Result<> TcpSocket::setNonBlocking(bool nb) {
#ifdef GEODE_IS_WINDOWS
    unsigned long mode = nb ? 1 : 0;
    if (SOCKET_ERROR == ioctlsocket(socket_, FIONBIO, &mode)) return Err(util::net::lastErrorString());
#else
    int flags = fcntl(socket_, F_GETFL);

    if (nb) {
        if (fcntl(socket_, F_SETFL, flags | O_NONBLOCK) < 0) return Err(util::net::lastErrorString());
    } else {
        if (fcntl(socket_, F_SETFL, flags & (~O_NONBLOCK)) < 0) return Err(util::net::lastErrorString());
    }
#endif

    return Ok();
}

void TcpSocket::maybeDisconnect() {
    auto lastError = util::net::lastErrorCode();

#ifdef GEODE_IS_WINDOWS
    if (lastError != WSAEWOULDBLOCK) {
        this->close();
    }
#else
    if (lastError != EWOULDBLOCK) {
        this->close();
    }
#endif
}
