#include "socket.hpp"
#include <util/net.hpp>

int Socket::send(const std::string& data) {
    return send(data.c_str(), data.size());
}

bool Socket::close() {
    return false;
}

Socket::~Socket() {
    this->close();
}