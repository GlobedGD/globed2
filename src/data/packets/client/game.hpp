#pragma once
#include <data/packets/packet.hpp>
#include <data/types/gd.hpp>

class RequestPlayerProfilesPacket : public Packet {
    GLOBED_PACKET(12000, false)

    GLOBED_PACKET_ENCODE {
        buf.writeI32(requested);
    }

    GLOBED_PACKET_DECODE_UNIMPL

    RequestPlayerProfilesPacket(int requested) : requested(requested) {}

    static std::shared_ptr<Packet> create(int requested) {
        return std::make_shared<RequestPlayerProfilesPacket>(requested);
    }

    int requested;
};

class LevelJoinPacket : public Packet {
    GLOBED_PACKET(12001, false)

    GLOBED_PACKET_ENCODE {
        buf.writeI32(levelId);
    }

    GLOBED_PACKET_DECODE_UNIMPL

    LevelJoinPacket(int levelId) : levelId(levelId) {}

    static std::shared_ptr<Packet> create(int levelId) {
        return std::make_shared<LevelJoinPacket>(levelId);
    }

    int levelId;
};

class LevelLeavePacket : public Packet {
    GLOBED_PACKET(12002, false)

    GLOBED_PACKET_ENCODE {}
    GLOBED_PACKET_DECODE_UNIMPL

    LevelLeavePacket() {}

    static std::shared_ptr<Packet> create() {
        return std::make_shared<LevelLeavePacket>();
    }
};

class PlayerDataPacket : public Packet {
    GLOBED_PACKET(12003, false)

    GLOBED_PACKET_ENCODE {
        buf.writeValue(data);
    }

    GLOBED_PACKET_DECODE_UNIMPL

    PlayerDataPacket(const PlayerData& data) : data(data) {}

    static std::shared_ptr<Packet> create(const PlayerData& data) {
        return std::make_shared<PlayerDataPacket>(data);
    }

    PlayerData data;
};

#if GLOBED_VOICE_SUPPORT

#include <audio/frame.hpp>

class VoicePacket : public Packet {
    GLOBED_PACKET(12010, true)

    GLOBED_PACKET_ENCODE {
        buf.writeValue(*frame.get());
    }

    GLOBED_PACKET_DECODE_UNIMPL

    VoicePacket(std::shared_ptr<EncodedAudioFrame> _frame) : frame(std::move(_frame)) {}

    static std::shared_ptr<Packet> create(std::shared_ptr<EncodedAudioFrame> frame) {
        return std::make_shared<VoicePacket>(std::move(frame));
    }

    std::shared_ptr<EncodedAudioFrame> frame;
};

#endif // GLOBED_VOICE_SUPPORT

class ChatMessagePacket : public Packet {
    GLOBED_PACKET(12011, true)

    GLOBED_PACKET_ENCODE {
        buf.writeString(message);
    }

    GLOBED_PACKET_DECODE_UNIMPL

    ChatMessagePacket(const std::string& message) : message(message) {}

    static std::shared_ptr<Packet> create(const std::string& message) {
        return std::make_shared<ChatMessagePacket>(message);
    }

    std::string message;
};