#include "tcp_socket.hpp"

#include "address.hpp"
#include <managers/settings.hpp>
#include <asp/time/Instant.hpp>
#include <util/net.hpp>

#ifdef GEODE_IS_WINDOWS
# include <WinSock2.h>
#else
# include <netinet/in.h>
# include <netinet/tcp.h>
# include <sys/socket.h>
# include <fcntl.h>
# include <poll.h>
# include <unistd.h>
#endif

#ifdef GEODE_IS_WINDOWS
constexpr static int WouldBlock = WSAEWOULDBLOCK;
#else
constexpr static int WouldBlock = EINPROGRESS;
#endif

using namespace geode::prelude;

TcpSocket::TcpSocket() : socket_(-1) {
    destAddr_ = std::make_unique<sockaddr_in>();
    std::memset(destAddr_.get(), 0, sizeof(sockaddr_in));

    globed::netLog("TcpSocket(this={}) created", (void*)this);
}

TcpSocket::~TcpSocket() {
    globed::netLog("TcpSocket(this={}) destroyed", (void*)this);
    this->close();
}

Result<> TcpSocket::connect(const NetworkAddress& address) {
    globed::netLog("TcpSocket::connect(this={}, address={})", (void*)this, GLOBED_LAZY(address.toString()));

    // close any socket if still open
    this->close();

    destAddr_->sin_family = AF_INET;

    GLOBED_UNWRAP_INTO(address.resolve(), *destAddr_)

    // create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    socket_ = sock;

    globed::netLog("TcpSocket::connect(this={}) bound to fd: {}", (void*)this, socket_.load());

    GLOBED_REQUIRE_SAFE(sock != -1, "failed to create a tcp socket: socket failed");

    // disable nagle algorithm for sends
    GLOBED_UNWRAP(this->setNodelay(true));

    // attempt a connection with a 5 second timeout
    GLOBED_UNWRAP(this->setNonBlocking(true));

    globed::netLog("TcpSocket::connect(this={}) attempting connect call", (void*)this);

    int code = ::connect(socket_, reinterpret_cast<struct sockaddr*>(destAddr_.get()), sizeof(sockaddr_in));

    // if the code isn't 0 (success) or EWOULDBLOCK (expected result), close socket and return error
    if (code != 0 && util::net::lastErrorCode() != WouldBlock) {
        auto errmsg = util::net::lastErrorString();
        globed::netLog("TcpSocket::connect(this={}) connect call failed, code {}: {}", (void*)this, code, errmsg);

        this->close();

        return Err(fmt::format("tcp connect failed ({}): {}", code, errmsg));
    }

    GLOBED_UNWRAP(this->setNonBlocking(false));

    // if the connection succeeded without blocking (local connection?), just return
    if (code == 0) {
        globed::netLog("TcpSocket::connect(this={}) connect call completed instantly, returning success!", (void*)this);
        return Ok();
    }

    globed::netLog("TcpSocket::connect(this={}) polling!", (void*)this);
    auto start = asp::time::Instant::now();

    GLOBED_UNWRAP_INTO(this->poll(5000, false), auto pollResult);

    globed::netLog(
        "TcpSocket::connect(this={}) poll returned after {}, result: {}",
        (void*)this, GLOBED_LAZY(start.elapsed().toString()), pollResult
    );

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

    globed::netLog("TcpSocket::send(this={}, fd={}, data={}, size={})", (void*)this, socket_.load(), (void*)data, dataSize);

    auto result = ::send(socket_, data, dataSize, flags);
    if (result == -1) {
        auto code = util::net::lastErrorCode();
        globed::netLog("TcpSocket::send(this={}) failed: code {}", (void*)this, code);

        this->maybeDisconnect();

        return Err(fmt::format("tcp send failed: {}", util::net::lastErrorString(code)));
    }

    return Ok(result);
}

Result<> TcpSocket::sendAll(const char* data, unsigned int dataSize) {
    globed::netLog("TcpSocket::sendAll(this={}, fd={}, data={}, size={})", (void*)this, socket_.load(), (void*)data, dataSize);

    unsigned int totalSent = 0;

    do {
        auto result_ = this->send(data + totalSent, dataSize - totalSent);
        GLOBED_UNWRAP_INTO(result_, auto result);

        totalSent += result;
    } while (totalSent < dataSize);

    return Ok();
}

void TcpSocket::disconnect() {
    globed::netLog("TcpSocket::disconnect(this={})", (void*)this);

    this->close();
}

RecvResult TcpSocket::receive(char* buffer, int bufferSize) {
    globed::netLog("TcpSocket::receive(this={}, connected={}, buf={}, size={})", (void*)this, connected.load(), (void*)buffer, bufferSize);

    if (!connected) {
        return RecvResult {
            .fromServer = true,
            .result = -1
        };
    }

    int result = ::recv(socket_, buffer, bufferSize, 0);

    globed::netLog("TcpSocket::receive recv() result: {}", result);

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
    globed::netLog("TcpSocket::recvExact(this={}, connected={}, buf={}, size={})", (void*)this, connected.load(), (void*)buffer, bufferSize);

    GLOBED_REQUIRE_SAFE(connected, "attempting to call TcpSocket::recvExact on a disconnected socket")

    unsigned int received = 0;

    do {
        int result = this->receive(buffer + received, bufferSize - received).result;
        if (result < 0) return Err(fmt::format("tcp recv failed ({}): {}", result, util::net::lastErrorString()));
        if (result == 0) return Err("connection was closed by the server");
        received += result;
    } while (received < bufferSize);

    return Ok();
}

bool TcpSocket::close() {
    connected = false;

    if (socket_ == -1) {
        return false;
    }

    globed::netLog("TcpSocket::close(this={}, fd = {})", (void*)this, socket_.load());

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
        return Err(fmt::format("tcp poll failed: {}", util::net::lastErrorString()));
    }

    return Ok(result > 0);
}

Result<> TcpSocket::setNonBlocking(bool nb) {
#ifdef GEODE_IS_WINDOWS
    unsigned long mode = nb ? 1 : 0;
    if (SOCKET_ERROR == ioctlsocket(socket_, FIONBIO, &mode)) return Err(fmt::format("ioctlsocket failed: {}", util::net::lastErrorString()));
#else
    int flags = fcntl(socket_, F_GETFL);

    if (nb) {
        if (fcntl(socket_, F_SETFL, flags | O_NONBLOCK) < 0) return Err(fmt::format("fcntl(O_NONBLOCK) failed: {}", util::net::lastErrorString()));
    } else {
        if (fcntl(socket_, F_SETFL, flags & (~O_NONBLOCK)) < 0) return Err(fmt::format("fcntl(~O_NONBLOCK) failed: {}", util::net::lastErrorString()));
    }
#endif

    return Ok();
}

Result<> TcpSocket::setNodelay(bool nodelay) {
    globed::netLog("TcpSocket::setNodelay(this={}, nodelay={})", (void*)this, nodelay);

    // enable/disable nagle's algorithm
    int flag = nodelay ? 1 : 0;
    if (setsockopt(socket_, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&flag), sizeof(flag)) < 0) {
        auto code = util::net::lastErrorCode();
        globed::netLog("TcpSocket::setNodelay(this={}) setsockopt(TCP_NODELAY) failed: {}", (void*)this, util::net::lastErrorString(code));
        return Err(fmt::format("setsockopt(TCP_NODELAY) failed: {}", util::net::lastErrorString(code)));
    }

    return Ok();
}

void TcpSocket::maybeDisconnect() {
    auto lastError = util::net::lastErrorCode();

    if (lastError != WouldBlock) {
        this->close();
    }
}
