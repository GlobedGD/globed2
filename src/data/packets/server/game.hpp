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

class VoiceBroadcastPacket : public Packet {
    GLOBED_PACKET(21010, true)

    GLOBED_PACKET_ENCODE_UNIMPL

    GLOBED_PACKET_DECODE {
        frame = buf.readValueUnique<EncodedAudioFrame>();
    }

    std::unique_ptr<EncodedAudioFrame> frame;
};

#endif // GLOBED_VOICE_SUPPORT