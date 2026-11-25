#pragma once
#include "../util/hash.hpp"
#include <stdint.h>
#include <string>
#include <optional>
#include <fmt/format.h>

namespace globed {

static constexpr uint32_t NS_EDITOR_COLLAB = 1;

/// A composite namespaced level ID, for support for non-GD levels
/// Regular GD levels have ns == 0 and id as the level ID on the GD servers
/// Other mods like editorcollab have nonzero ns and custom opaque 64-bit ID.
struct LevelId {
    uint64_t id = 0;
    uint32_t ns = 0;

    static LevelId fromVanilla(int levelId) {
        return {
            .id = (uint64_t)levelId,
            .ns = 0
        };
    }

    static LevelId fromLegacyEC(int64_t id) {
        return {
            .id = (uint64_t)id,
            .ns = NS_EDITOR_COLLAB
        };
    }

    bool isZero() const {
        return ns == 0 && id == 0;
    }

    bool isVanillaLevel(int levelId) const {
        return ns == 0 && id == (uint64_t)levelId;
    }

    bool isVanillaLevel() const {
        return ns == 0;
    }

    bool operator==(const LevelId& other) const = default;
};

struct SessionId {
    SessionId() = default;
    SessionId(LevelId level, uint8_t serverId, uint32_t roomId) : level(level), roomId(roomId), serverId(serverId) {}

    LevelId level;
    uint32_t roomId = 0;
    uint8_t serverId = 0;

    size_t hash() const {
        size_t seed = 1;
        hash_combine(seed, level.ns, level.id, roomId, serverId);
        return seed;
    }

    /// Session ID for central server
    std::optional<uint64_t> asLegacyVanillaId() const {
        if (level.ns != 0) return std::nullopt;

        uint64_t a = static_cast<uint64_t>(serverId) << 56;
        uint64_t b = static_cast<uint64_t>(roomId & 0x00ffffff) << 32;
        uint64_t c = static_cast<uint32_t>(level.id);
        return a | b | c;
    }

    static SessionId fromLegacy(uint64_t value) {
        uint8_t serverId = (value >> 56) & 0xff;
        uint32_t roomId = (value >> 32) & 0x00ffffff;
        uint32_t levelId = value & 0xffffffff;

        return SessionId(LevelId::fromVanilla(levelId), serverId, roomId);
    }

    bool isZero() const {
        return level.isZero() && roomId == 0 && serverId == 0;
    }

    bool isVanillaLevel() const {
        return level.isVanillaLevel();
    }

    bool isVanillaLevel(int levelId) const {
        return level.isVanillaLevel(levelId);
    }

    bool operator==(const SessionId& other) const = default;
};

inline std::string format_as(const SessionId& sid) {
    return fmt::format("SessionId(ns={}, id={}, room={}, server={})", sid.level.ns, sid.level.id, sid.roomId, sid.serverId);
}

}

namespace std {
    template <>
    struct hash<globed::SessionId> {
        static size_t operator()(const globed::SessionId& sid) {
            return sid.hash();
        }
    };
}
