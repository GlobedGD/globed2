#pragma once
#include <stdint.h>

namespace globed {

class SessionId {
public:
    inline explicit SessionId() : value(0) {}
    inline explicit SessionId(uint64_t id) : value(id) {}
    inline SessionId(const SessionId& other) : value(other.value) {}

    inline SessionId& operator=(const SessionId& other) {
        value = other.value;
        return *this;
    }

    static inline SessionId fromParts(uint8_t serverId, uint32_t roomId, int levelId) {
        uint64_t a = static_cast<uint64_t>(serverId) << 56;
        uint64_t b = static_cast<uint64_t>(roomId & 0x00ffffff) << 32;
        uint64_t c = static_cast<uint32_t>(levelId);
        return SessionId(a | b | c);
    }

    operator uint64_t() const {
        return value;
    }

    uint64_t asU64() const {
        return value;
    }

    uint8_t serverId() const {
        return static_cast<uint8_t>(value >> 56);
    }

    int levelId() const {
        return static_cast<int>(value & 0xffffffffULL);
    }

    uint32_t roomId() const {
        return static_cast<uint32_t>((value >> 32) & 0x00ffffffULL);
    }

private:
    uint64_t value;
};

}

namespace std {
    template <>
    struct hash<globed::SessionId> {
        static size_t operator()(const globed::SessionId& sid) {
            return std::hash<uint64_t>{}(sid.asU64());
        }
    };
}
