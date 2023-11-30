#include "socket.hpp"
#include <util/net.hpp>

int Socket::send(const std::string& data) {
    return send(data.c_str(), data.size());
}

void Socket::receiveExact(char* buffer, int bufferSize) {
    char* bufptr = buffer;

    do {
        auto received = receive(bufptr, bufferSize - (bufptr - buffer));
        if (received == -1) {
            util::net::throwLastError();
        }
        bufptr += received;
    } while (buffer + bufferSize > bufptr);
}

bool Socket::close() {
    return false;
}

Socket::~Socket() {
    close();
}