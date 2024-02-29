#pragma once
#include <data/packets/packet.hpp>
#include <data/types/gd.hpp>

class RequestPlayerProfilesPacket : public Packet {
    GLOBED_PACKET(12000, false, false)

    GLOBED_PACKET_ENCODE {
        buf.writeI32(requested);
    }

    RequestPlayerProfilesPacket(int requested) : requested(requested) {}

    static std::shared_ptr<Packet> create(int requested) {
        return std::make_shared<RequestPlayerProfilesPacket>(requested);
    }

    int requested;
};

class LevelJoinPacket : public Packet {
    GLOBED_PACKET(12001, false, false)

    GLOBED_PACKET_ENCODE {
        buf.writePrimitive(levelId);
    }

    LevelJoinPacket(LevelId levelId) : levelId(levelId) {}

    static std::shared_ptr<Packet> create(LevelId levelId) {
        return std::make_shared<LevelJoinPacket>(levelId);
    }

    LevelId levelId;
};

class LevelLeavePacket : public Packet {
    GLOBED_PACKET(12002, false, false)

    GLOBED_PACKET_ENCODE {}

    LevelLeavePacket() {}

    static std::shared_ptr<Packet> create() {
        return std::make_shared<LevelLeavePacket>();
    }
};

class PlayerDataPacket : public Packet {
    GLOBED_PACKET(12003, false, false)

    GLOBED_PACKET_ENCODE {
        buf.writeValue(data);
    }

    PlayerDataPacket(const PlayerData& data) : data(data) {}

    static std::shared_ptr<Packet> create(const PlayerData& data) {
        return std::make_shared<PlayerDataPacket>(data);
    }

    PlayerData data;
};

class PlayerMetadataPacket : public Packet {
    GLOBED_PACKET(12004, false, false)

    GLOBED_PACKET_ENCODE {
        buf.writeValue(data);
    }

    PlayerMetadataPacket(const PlayerMetadata& data) : data(data) {}

    static std::shared_ptr<Packet> create(const PlayerMetadata& data) {
        return std::make_shared<PlayerMetadataPacket>(data);
    }

    PlayerMetadata data;
};

#if GLOBED_VOICE_SUPPORT

#include <audio/frame.hpp>

class VoicePacket : public Packet {
    GLOBED_PACKET(12010, true, false)

    GLOBED_PACKET_ENCODE {
        buf.writeValue(*frame.get());
    }

    VoicePacket(std::shared_ptr<EncodedAudioFrame> _frame) : frame(_frame) {}

    static std::shared_ptr<Packet> create(std::shared_ptr<EncodedAudioFrame> frame) {
        return std::make_shared<VoicePacket>(frame);
    }

    std::shared_ptr<EncodedAudioFrame> frame;
};

#endif // GLOBED_VOICE_SUPPORT

class ChatMessagePacket : public Packet {
    GLOBED_PACKET(12011, true, false)

    GLOBED_PACKET_ENCODE {
        buf.writeString(message);
    }

    ChatMessagePacket(const std::string_view message) : message(message) {}

    static std::shared_ptr<Packet> create(const std::string_view message) {
        return std::make_shared<ChatMessagePacket>(message);
    }

    std::string message;
};