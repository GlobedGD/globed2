#pragma once
#include <data/packets/packet.hpp>
#include <data/types/gd.hpp>

class PlayerProfilesPacket : public Packet {
    GLOBED_PACKET(21000, false)

    GLOBED_PACKET_ENCODE_UNIMPL

    GLOBED_PACKET_DECODE {
        data = buf.readValueVector<PlayerAccountData>();
    }

    std::vector<PlayerAccountData> data;
};

class LevelDataPacket : public Packet {
    GLOBED_PACKET(21001, false)

    GLOBED_PACKET_ENCODE_UNIMPL

    GLOBED_PACKET_DECODE {
        players = buf.readValueVector<AssociatedPlayerData>();
    }

    std::vector<AssociatedPlayerData> players;
};

class PlayerListPacket : public Packet {
    GLOBED_PACKET(21002, false)

    GLOBED_PACKET_ENCODE_UNIMPL

    GLOBED_PACKET_DECODE {
        data = buf.readValueVector<PlayerPreviewAccountData>();
    }

    std::vector<PlayerPreviewAccountData> data;
};

class PlayerMetadataPacket : public Packet {
    GLOBED_PACKET(21003, false)

    GLOBED_PACKET_ENCODE_UNIMPL

    GLOBED_PACKET_DECODE {
        players = buf.readValueVector<AssociatedPlayerMetadata>();
    }

    std::vector<AssociatedPlayerMetadata> players;
};

#if GLOBED_VOICE_SUPPORT
#include <audio/audio_frame.hpp>
#endif

class VoiceBroadcastPacket : public Packet {
    GLOBED_PACKET(21010, true)

    GLOBED_PACKET_ENCODE_UNIMPL

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
    GLOBED_PACKET(21011, true)

    GLOBED_PACKET_ENCODE_UNIMPL

    GLOBED_PACKET_DECODE {
        sender = buf.readI32();
        message = buf.readString();
    }

    int sender;
    std::string message;
};