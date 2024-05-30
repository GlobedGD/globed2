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
    LevelJoinPacket(LevelId levelId) : levelId(levelId) {}

    LevelId levelId;
};

GLOBED_SERIALIZABLE_STRUCT(LevelJoinPacket, (levelId));

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
    PlayerDataPacket(const PlayerData& data) : data(data) {}

    PlayerData data;
};

GLOBED_SERIALIZABLE_STRUCT(PlayerDataPacket, (data));

// 12004 - PlayerMetadataPacket
class PlayerMetadataPacket : public Packet {
    GLOBED_PACKET(12004, PlayerMetadataPacket, false, false)

    PlayerMetadataPacket() {}
    PlayerMetadataPacket(const PlayerMetadata& data) : data(data) {}

    PlayerMetadata data;
};

GLOBED_SERIALIZABLE_STRUCT(PlayerMetadataPacket, (data));

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
    ChatMessagePacket(const std::string_view message) : message(message) {}

    std::string message;
};

GLOBED_SERIALIZABLE_STRUCT(ChatMessagePacket, (message));
