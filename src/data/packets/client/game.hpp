#pragma once
#include <data/packets/packet.hpp>
#include <data/types/gd.hpp>

// 12000 - RequestPlayerProfilesPacket
class RequestPlayerProfilesPacket : public Packet {
    GLOBED_PACKET(12000, RequestPlayerProfilesPacket, false, false)

    RequestPlayerProfilesPacket() {}
    RequestPlayerProfilesPacket(int requested) : requested(requested) {}

    int requested;
};
GLOBED_SERIALIZABLE_STRUCT(RequestPlayerProfilesPacket, (requested));

// 12001 - LevelJoinPacket
class LevelJoinPacket : public Packet {
    GLOBED_PACKET(12001, LevelJoinPacket, false, false)

    LevelJoinPacket() {}
    LevelJoinPacket(LevelId levelId, bool unlisted, std::optional<std::array<uint8_t, 32>> levelHash) : levelId(levelId), unlisted(unlisted), levelHash(levelHash) {}

    LevelId levelId;
    bool unlisted;
    std::optional<std::array<uint8_t, 32>> levelHash;
};
GLOBED_SERIALIZABLE_STRUCT(LevelJoinPacket, (levelId, unlisted, levelHash));

// 12002 - LevelLeavePacket
class LevelLeavePacket : public Packet {
    GLOBED_PACKET(12002, LevelLeavePacket, false, false)

    LevelLeavePacket() {}
};
GLOBED_SERIALIZABLE_STRUCT(LevelLeavePacket, ());

// 12003 - PlayerDataPacket
class PlayerDataPacket : public Packet {
    GLOBED_PACKET(12003, PlayerDataPacket, false, false)

    PlayerDataPacket() {}
    PlayerDataPacket(const PlayerData& data, const std::optional<PlayerMetadata>& meta, std::vector<GlobedCounterChange>&& counterChanges) : data(data), meta(meta), counterChanges(std::move(counterChanges)) {}

    PlayerData data;
    std::optional<PlayerMetadata> meta;
    std::vector<GlobedCounterChange> counterChanges;
};

template <>
inline ByteBuffer::DecodeResult<PlayerDataPacket> ByteBuffer::customDecode<PlayerDataPacket>() {
    throw std::runtime_error("unreachable tbh");
}

template <>
inline void ByteBuffer::customEncode<PlayerDataPacket>(const PlayerDataPacket& packet) {
    this->writeValue(packet.data);
    this->writeValue(packet.meta);

    this->writeU8(packet.counterChanges.size());

    for (const auto& change : packet.counterChanges) {
        this->writeValue(change);
    }
}

#ifdef GLOBED_VOICE_SUPPORT

#include <audio/frame.hpp>

// 12010 - VoicePacket
class VoicePacket : public Packet {
    GLOBED_PACKET(12010, VoicePacket, true, false)

    VoicePacket() {}
    VoicePacket(std::shared_ptr<EncodedAudioFrame> _frame) : frame(_frame) {}

    std::shared_ptr<EncodedAudioFrame> frame;
};
GLOBED_SERIALIZABLE_STRUCT(VoicePacket, (frame));

#endif // GLOBED_VOICE_SUPPORT

// 12011 - ChatMessagePacket
class ChatMessagePacket : public Packet {
    GLOBED_PACKET(12011, ChatMessagePacket, true, false)

    ChatMessagePacket() {}
    ChatMessagePacket(std::string_view message) : message(message) {}

    std::string message;
};
GLOBED_SERIALIZABLE_STRUCT(ChatMessagePacket, (message));
