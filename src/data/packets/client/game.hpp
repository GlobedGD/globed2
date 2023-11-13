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

    static SyncIconsPacket* create(const PlayerIconData& icons) {
        return new SyncIconsPacket(icons);
    }

    PlayerIconData icons;
};

class RequestProfilesPacket : public Packet {
    GLOBED_PACKET(11001, false)

    GLOBED_PACKET_ENCODE {
        buf.writeU32(ids.size());
        for (auto id : ids) {
            buf.writeI32(id);
        }
    }

    GLOBED_PACKET_DECODE_UNIMPL

    RequestProfilesPacket(const std::vector<int32_t>& ids) : ids(ids) {}

    static RequestProfilesPacket* create(const std::vector<int32_t>& ids) {
        return new RequestProfilesPacket(ids);
    }

    std::vector<int32_t> ids;
};

#if GLOBED_VOICE_SUPPORT

#include <audio/audio_frame.hpp>

class VoicePacket : public Packet {
    GLOBED_PACKET(11010, true)

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