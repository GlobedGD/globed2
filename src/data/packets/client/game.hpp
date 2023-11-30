#pragma once
#include <defs.hpp>
#include <data/packets/packet.hpp>
#include <data/types/gd.hpp>

class SyncIconsPacket : public Packet {
    GLOBED_PACKET(11000, false)

    GLOBED_PACKET_ENCODE {
        buf.writeValue(icons);
    }

    GLOBED_PACKET_DECODE_UNIMPL

    SyncIconsPacket(const PlayerIconData& icons) : icons(icons) {}

    static std::shared_ptr<Packet> create(const PlayerIconData& icons) {
        return std::make_shared<SyncIconsPacket>(icons);
    }

    PlayerIconData icons;
};

constexpr size_t MAX_PROFILES_REQUESTED = 128;
class RequestProfilesPacket : public Packet {
    GLOBED_PACKET(11001, false)

    GLOBED_PACKET_ENCODE {
        for (auto id : ids) {
            buf.writeI32(id);
        }
    }

    GLOBED_PACKET_DECODE_UNIMPL

    RequestProfilesPacket(const std::array<int, MAX_PROFILES_REQUESTED>& ids) : ids(ids) {}

    static std::shared_ptr<Packet> create(const std::array<int, MAX_PROFILES_REQUESTED>& ids) {
        return std::make_shared<RequestProfilesPacket>(ids);
    }

    std::array<int, MAX_PROFILES_REQUESTED> ids;
};

class LevelJoinPacket : public Packet {
    GLOBED_PACKET(11002, false)

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
    GLOBED_PACKET(11003, false)

    GLOBED_PACKET_ENCODE {}
    GLOBED_PACKET_DECODE_UNIMPL

    LevelLeavePacket() {}

    static std::shared_ptr<Packet> create() {
        return std::make_shared<LevelLeavePacket>();
    }
};

class PlayerDataPacket : public Packet {
    GLOBED_PACKET(11004, false)

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

class RequestPlayerListPacket : public Packet {
    GLOBED_PACKET(11005, false)

    GLOBED_PACKET_ENCODE {}
    GLOBED_PACKET_DECODE_UNIMPL

    RequestPlayerListPacket() {}

    static std::shared_ptr<Packet> create() {
        return std::make_shared<RequestPlayerListPacket>();
    }
};

class SyncPlayerMetadataPacket : public Packet {
    GLOBED_PACKET(11006, false)

    GLOBED_PACKET_ENCODE {
        buf.writeValue(data);
    }

    GLOBED_PACKET_DECODE_UNIMPL

    SyncPlayerMetadataPacket(const PlayerMetadata& data) : data(data) {}

    static std::shared_ptr<Packet> create(const PlayerMetadata& data) {
        return std::make_shared<SyncPlayerMetadataPacket>(data);
    }

    PlayerMetadata data;
};

#if GLOBED_VOICE_SUPPORT

#include <audio/audio_frame.hpp>

class VoicePacket : public Packet {
    GLOBED_PACKET(11010, true)

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
    GLOBED_PACKET(11011, true)

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