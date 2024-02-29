#pragma once
#include <data/packets/packet.hpp>
#include <data/types/gd.hpp>

class PlayerProfilesPacket : public Packet {
    GLOBED_PACKET(22000, false, false)

    GLOBED_PACKET_DECODE {
        players = buf.readValueVector<PlayerAccountData>();
    }

    std::vector<PlayerAccountData> players;
};

class LevelDataPacket : public Packet {
    GLOBED_PACKET(22001, false, false)

    GLOBED_PACKET_DECODE {
        players = buf.readValueVector<AssociatedPlayerData>();
    }

    std::vector<AssociatedPlayerData> players;
};

class LevelPlayerMetadataPacket : public Packet {
    GLOBED_PACKET(22002, false, false)

    GLOBED_PACKET_DECODE {
        players = buf.readValueVector<AssociatedPlayerMetadata>();
    }

    std::vector<AssociatedPlayerMetadata> players;
};

#if GLOBED_VOICE_SUPPORT
# include <audio/frame.hpp>
#endif

class VoiceBroadcastPacket : public Packet {
    GLOBED_PACKET(22010, true, false)

#if GLOBED_VOICE_SUPPORT
    GLOBED_PACKET_DECODE {
        sender = buf.readI32();
        frame = buf.readValue<EncodedAudioFrame>();
    }

    int sender;
    EncodedAudioFrame frame;
#else
    GLOBED_PACKET_DECODE {}
#endif // GLOBED_VOICE_SUPPORT
};

class ChatMessageBroadcastPacket : public Packet {
    GLOBED_PACKET(22011, true, false)

    GLOBED_PACKET_DECODE {
        sender = buf.readI32();
        message = buf.readString();
    }

    int sender;
    std::string message;
};
