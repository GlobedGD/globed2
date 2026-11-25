#pragma once
#include "../util/hash.hpp"
#include <stdint.h>

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
};

struct SessionId {
    SessionId() = default;
    SessionId(LevelId level, uint8_t serverId, uint32_t roomId) : level(level), serverId(serverId), roomId(roomId) {}

    // static SessionId fromLegacy(uint64_t value) {

    // }

    LevelId level;
    uint32_t roomId;
    uint8_t serverId;

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

    bool isZero() const {
        return level.isZero() && roomId == 0 && serverId == 0;
    }
};

}

namespace std {
    template <>
    struct hash<globed::SessionId> {
        static size_t operator()(const globed::SessionId& sid) {
            return std::hash<uint64_t>{}(sid.hash());
        }
    };
}
