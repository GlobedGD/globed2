#pragma once
#include <defs.hpp>
#include <data/packets/packet.hpp>
#include <data/types/gd.hpp>

class PlayerProfilesPacket : public Packet {
    GLOBED_PACKET(21000, false)

    GLOBED_PACKET_ENCODE_UNIMPL

    GLOBED_PACKET_DECODE {
        auto values = buf.readValueVector<PlayerAccountData>();
        for (const auto& val : values) {
            data[val.id] = val;
        }
    }

    std::unordered_map<int32_t, PlayerAccountData> data;
};

#if GLOBED_VOICE_SUPPORT
#include <audio/audio_frame.hpp>
#endif

class VoiceBroadcastPacket : public Packet {
    GLOBED_PACKET(21010, true)

    GLOBED_PACKET_ENCODE_UNIMPL
    
#if GLOBED_VOICE_SUPPORT
    GLOBED_PACKET_DECODE {
        frame = std::move(buf.readValue<EncodedAudioFrame>());
    }

    EncodedAudioFrame frame;
#else
    GLOBED_PACKET_DECODE {}
#endif // GLOBED_VOICE_SUPPORT
};
