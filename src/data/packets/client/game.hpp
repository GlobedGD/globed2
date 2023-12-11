#pragma once
#include <data/packets/packet.hpp>
#include <data/types/gd.hpp>

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

class SyncPlayerMetadataPacket : public Packet {
    GLOBED_PACKET(12004, false)

    GLOBED_PACKET_ENCODE {
        buf.writeValue(data);

        // what a dogshit fucking language that refuses to work unless i write down a whole essay in my variable declaration
        std::function<void(ByteBuffer&, const int&)> ef = [](auto& buf, const int& val) {
            buf.writeI32(val);
        };

        buf.writeOptionalValue(requested, ef);
    }

    GLOBED_PACKET_DECODE_UNIMPL

    SyncPlayerMetadataPacket(const PlayerMetadata& data, const std::optional<int>& requested) : data(data), requested(requested) {}

    static std::shared_ptr<Packet> create(const PlayerMetadata& data, const std::optional<int>& requested) {
        return std::make_shared<SyncPlayerMetadataPacket>(data, requested);
    }

    PlayerMetadata data;
    std::optional<int> requested;
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