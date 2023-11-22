#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <ui/hooks/all.hpp>
#include <ui/error_check_node.hpp>

#include <crypto/box.hpp>
#include <data/bytebuffer.hpp>
#include <util/all.hpp>
#include <bit>

#include <audio/opus_codec.hpp>
#include <audio/audio_manager.hpp>
#include <audio/voice_playback_manager.hpp>
#include <discord/manager.hpp>
#include <net/network_manager.hpp>

#include <data/packets/all.hpp>
#include <managers/error_queues.hpp>
#include <managers/account_manager.hpp>

using namespace geode::prelude;

$on_mod(Loaded) {
    // if there is a logic error in the crypto code, sodium_misuse() gets called
    sodium_set_misuse_handler([](){
        log::error("sodium_misuse called. we are officially screwed.");
        util::debugging::suicide();
    });

    auto ecn = ErrorCheckNode::create();
    ecn->setID("error-check-node"_spr);
    SceneManager::get()->keepAcrossScenes(ecn);
    
#if defined(GLOBED_DEBUG) && GLOBED_DEBUG
    geode::log::warn("=== Globed v{} has been loaded in debug mode ===", Mod::get()->getVersion().toString());
    geode::log::info("Platform: {}", GLOBED_PLATFORM_STRING);
    geode::log::info("FMOD support: {}", GLOBED_HAS_FMOD == 0 ? "false" : "true");
    geode::log::info("Voice support: {}", GLOBED_VOICE_SUPPORT == 0 ? "false" : "true");
    geode::log::info("Discord RPC support: {}", GLOBED_HAS_DRPC == 0 ? "false" : "true");
    geode::log::info("Little endian: {}", GLOBED_LITTLE_ENDIAN ? "true" : "false");
    geode::log::info("Libsodium version: {}", SODIUM_VERSION_STRING);
    #if GLOBED_VOICE_SUPPORT
    geode::log::info("Opus version: {}", opus_get_version_string());
    #endif
#endif
}

void testFmod1();
void testFmod2();

class $modify(MyMenuLayer, MenuLayer) {
    void onMoreGames(CCObject*) {
        if (NetworkManager::get().established()) {
            util::debugging::PacketLogger::get().getSummary().print();
        }

        // nm.addListener<PingResponsePacket>([](auto* packet) {
        //     log::debug("got ping packet with id {}, pc: {}", packet->id, packet->playerCount);
        // });

        // nm.addListener(PingResponsePacket::PACKET_ID, [](std::shared_ptr<Packet> packet) {
        //     auto pkt = static_cast<PingResponsePacket*>(packet.get());
        //     log::debug("got ping packet with id {}, pc: {}", pkt->id, pkt->playerCount);
        // });

        // nm.connect("127.0.0.1", 41001);
        // nm.send(PingPacket::create(69696969));
        
        // DiscordManager::get().update();
        // auto& vm = GlobedAudioManager::get();
        // vm.setActiveRecordingDevice(2);
        // log::debug("Listening to: {}", vm.getRecordingDevice().name);

        // testFmod1();
        // testFmod2();

        // util::debugging::PacketLogger::get().getSummary().print();
        
    }
};

#if GLOBED_VOICE_SUPPORT
// following listens to a frame and immediately plays it
void testFmod1() {
    auto& vm = GlobedAudioManager::get();
    vm.startRecording([&vm](const EncodedAudioFrame& frame){
        util::debugging::Benchmarker bench;

        ByteBuffer bb;
        bb.writeValue(frame);

        bb.setPosition(0);

        auto decodedFrame = bb.readValue<EncodedAudioFrame>();
        VoicePlaybackManager::get().playFrameStreamed(0, decodedFrame);
    });
}

// following joins 5 different frames into one sound and plays it
void testFmod2() {
    auto& vm = GlobedAudioManager::get();

    ByteBuffer* bb = new ByteBuffer;
    int* total = new int{0};
    
    vm.startRecording([&vm, bb, total](const EncodedAudioFrame& frame){
        *total = *total + 1;

        bb->writeValue(frame);
        log::debug("Frame {} recorded", static_cast<int>(*total));

        if (*total > 5) { // end after 5 frames
            vm.queueStopRecording();
            bb->setPosition(0);
            std::vector<float> out;
            for (size_t i = 0; i < *total; i++) {
                log::debug("Reading frame {}", i);
                auto encFrame = bb->readValue<EncodedAudioFrame>();
                const auto& opusFrames = encFrame.getFrames();
                
                for (const auto& opusFrame : opusFrames) {
                    auto rawFrame = vm.decodeSound(opusFrame);
                    out.insert(out.end(), rawFrame.ptr, rawFrame.ptr + rawFrame.length);
                }
            }

            log::debug("total pcm samples: {}", out.size());

            auto sound = vm.createSound(out.data(), out.size());
            vm.playSound(sound);

        }
    });
}
#endif // GLOBED_VOICE_SUPPORT
