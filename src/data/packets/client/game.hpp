#pragma once
#include <data/packets/packet.hpp>
#include <data/types/gd.hpp>

class RequestPlayerProfilesPacket : public Packet {
    GLOBED_PACKET(12000, false, false)

    RequestPlayerProfilesPacket() {}
    RequestPlayerProfilesPacket(int requested) : requested(requested) {}

    static std::shared_ptr<Packet> create(int requested) {
        return std::make_shared<RequestPlayerProfilesPacket>(requested);
    }

    int requested;
};

GLOBED_SERIALIZABLE_STRUCT(RequestPlayerProfilesPacket, (requested));

class LevelJoinPacket : public Packet {
    GLOBED_PACKET(12001, false, false)

    LevelJoinPacket() {}
    LevelJoinPacket(LevelId levelId) : levelId(levelId) {}

    static std::shared_ptr<Packet> create(LevelId levelId) {
        return std::make_shared<LevelJoinPacket>(levelId);
    }

    LevelId levelId;
};

GLOBED_SERIALIZABLE_STRUCT(LevelJoinPacket, (levelId));

class LevelLeavePacket : public Packet {
    GLOBED_PACKET(12002, false, false)

    LevelLeavePacket() {}

    static std::shared_ptr<Packet> create() {
        return std::make_shared<LevelLeavePacket>();
    }
};

GLOBED_SERIALIZABLE_STRUCT(LevelLeavePacket, ());

class PlayerDataPacket : public Packet {
    GLOBED_PACKET(12003, false, false)

    PlayerDataPacket() {}
    PlayerDataPacket(const PlayerData& data) : data(data) {}

    static std::shared_ptr<Packet> create(const PlayerData& data) {
        return std::make_shared<PlayerDataPacket>(data);
    }

    PlayerData data;
};

GLOBED_SERIALIZABLE_STRUCT(PlayerDataPacket, (data));

class PlayerMetadataPacket : public Packet {
    GLOBED_PACKET(12004, false, false)

    PlayerMetadataPacket() {}
    PlayerMetadataPacket(const PlayerMetadata& data) : data(data) {}

    static std::shared_ptr<Packet> create(const PlayerMetadata& data) {
        return std::make_shared<PlayerMetadataPacket>(data);
    }

    PlayerMetadata data;
};

GLOBED_SERIALIZABLE_STRUCT(PlayerMetadataPacket, (data));

#ifdef GLOBED_VOICE_SUPPORT

#include <audio/frame.hpp>

class VoicePacket : public Packet {
    GLOBED_PACKET(12010, true, false)

    VoicePacket() {}
    VoicePacket(std::shared_ptr<EncodedAudioFrame> _frame) : frame(_frame) {}

    static std::shared_ptr<Packet> create(std::shared_ptr<EncodedAudioFrame> frame) {
        return std::make_shared<VoicePacket>(frame);
    }

    std::shared_ptr<EncodedAudioFrame> frame;
};

GLOBED_SERIALIZABLE_STRUCT(VoicePacket, (frame));

#endif // GLOBED_VOICE_SUPPORT

class ChatMessagePacket : public Packet {
    GLOBED_PACKET(12011, true, false)

    ChatMessagePacket() {}
    ChatMessagePacket(const std::string_view message) : message(message) {}

    static std::shared_ptr<Packet> create(const std::string_view message) {
        return std::make_shared<ChatMessagePacket>(message);
    }

    std::string message;
};

GLOBED_SERIALIZABLE_STRUCT(ChatMessagePacket, (message));
