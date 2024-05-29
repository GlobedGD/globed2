#pragma once
#include <data/packets/packet.hpp>
#include <data/types/gd.hpp>

class PlayerProfilesPacket : public Packet {
    GLOBED_PACKET(22000, PlayerProfilesPacket, false, false)

    PlayerProfilesPacket() {}

    std::vector<PlayerAccountData> players;
};

GLOBED_SERIALIZABLE_STRUCT(PlayerProfilesPacket, (players));

class LevelDataPacket : public Packet {
    GLOBED_PACKET(22001, LevelDataPacket, false, false)

    LevelDataPacket() {}

    std::vector<AssociatedPlayerData> players;
};

GLOBED_SERIALIZABLE_STRUCT(LevelDataPacket, (players));

class LevelPlayerMetadataPacket : public Packet {
    GLOBED_PACKET(22002, LevelPlayerMetadataPacket, false, false)

    LevelPlayerMetadataPacket() {}

    std::vector<AssociatedPlayerMetadata> players;
};

GLOBED_SERIALIZABLE_STRUCT(LevelPlayerMetadataPacket, (players));

#ifdef GLOBED_VOICE_SUPPORT
# include <audio/frame.hpp>
#endif

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

class ChatMessageBroadcastPacket : public Packet {
    GLOBED_PACKET(22011, ChatMessageBroadcastPacket, true, false)

    ChatMessageBroadcastPacket() {}

    int sender;
    std::string message;
};

GLOBED_SERIALIZABLE_STRUCT(ChatMessageBroadcastPacket, (sender, message));
