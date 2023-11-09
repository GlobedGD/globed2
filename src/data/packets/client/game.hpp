#pragma once
#include <defs.hpp>
#include <data/packets/packet.hpp>

#if GLOBED_VOICE_SUPPORT

#include <audio/audio_frame.hpp>

class VoicePacket : public Packet {
    GLOBED_PACKET(11010, false)

    GLOBED_PACKET_ENCODE {
        buf.writeValue(*frame.get());
    }

    GLOBED_PACKET_DECODE_UNIMPL

    VoicePacket(std::unique_ptr<EncodedAudioFrame> _frame) : frame(std::move(_frame)) {}

    static VoicePacket* create(std::unique_ptr<EncodedAudioFrame> frame) {
        return new VoicePacket(std::move(frame));
    }

    std::unique_ptr<EncodedAudioFrame> frame;
};

#endif // GLOBED_VOICE_SUPPORT