#pragma once
#include <data/packets/packet.hpp>
#include <data/types/gd.hpp>

// 22000 - PlayerProfilesPacket
class PlayerProfilesPacket : public Packet {
    GLOBED_PACKET(22000, PlayerProfilesPacket, false, false)

    PlayerProfilesPacket() {}

    std::vector<PlayerAccountData> players;
};

GLOBED_SERIALIZABLE_STRUCT(PlayerProfilesPacket, (players));

// 22001 - LevelDataPacket
class LevelDataPacket : public Packet {
    GLOBED_PACKET(22001, LevelDataPacket, false, false)

    LevelDataPacket() {}

    std::vector<AssociatedPlayerData> players;
    std::optional<std::map<uint16_t, int>> customItems;
};

GLOBED_SERIALIZABLE_STRUCT(LevelDataPacket, (players, customItems));

// 22002 - LevelPlayerMetadataPacket
class LevelPlayerMetadataPacket : public Packet {
    GLOBED_PACKET(22002, LevelPlayerMetadataPacket, false, false)

    LevelPlayerMetadataPacket() {}

    std::vector<AssociatedPlayerMetadata> players;
};

GLOBED_SERIALIZABLE_STRUCT(LevelPlayerMetadataPacket, (players));

// 22003 - LevelInnerPlayerCountPacket
class LevelInnerPlayerCountPacket : public Packet {
    GLOBED_PACKET(22003, LevelInnerPlayerCountPacket, false, false)

    LevelInnerPlayerCountPacket() {}

    uint32_t count;
};

GLOBED_SERIALIZABLE_STRUCT(LevelInnerPlayerCountPacket, (count));

#ifdef GLOBED_VOICE_SUPPORT
# include <audio/frame.hpp>
#endif

// 22010 - VoiceBroadcastPacket
class VoiceBroadcastPacket : public Packet {
    GLOBED_PACKET(22010, VoiceBroadcastPacket, true, false)

    VoiceBroadcastPacket() {}

#ifdef GLOBED_VOICE_SUPPORT
    int sender;
    EncodedAudioFrame frame;
#endif
};

#ifdef GLOBED_VOICE_SUPPORT
    GLOBED_SERIALIZABLE_STRUCT(VoiceBroadcastPacket, (sender, frame));
#else
    GLOBED_SERIALIZABLE_STRUCT(VoiceBroadcastPacket, ());
#endif // GLOBED_VOICE_SUPPORT

// 22011 - ChatMessageBroadcastPacket
class ChatMessageBroadcastPacket : public Packet {
    GLOBED_PACKET(22011, ChatMessageBroadcastPacket, true, false)

    ChatMessageBroadcastPacket() {}

    int sender;
    std::string message;
};

GLOBED_SERIALIZABLE_STRUCT(ChatMessageBroadcastPacket, (sender, message));

// 22012 - VoiceFailedPacket
class VoiceFailedPacket : public Packet {
    GLOBED_PACKET(22012, VoiceFailedPacket, false, false)

    VoiceFailedPacket() {}

    bool userMuted;
};

GLOBED_SERIALIZABLE_STRUCT(VoiceFailedPacket, (userMuted));
