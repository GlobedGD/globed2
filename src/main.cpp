#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <UIBuilder.hpp>

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
    
#if defined(GLOBED_DEBUG) && GLOBED_DEBUG
    geode::log::warn("=== Globed has been loaded in debug mode ===");
#endif
}

void testFmod1();
void testFmod2();

class $modify(MyMenuLayer, MenuLayer) {
    bool init() {
        MenuLayer::init();

        CCScheduler::get()->scheduleSelector(schedule_selector(MyMenuLayer::Poop), this, 1.0f, false);
        return true;
    }

    void Poop(float d) {
        auto& eq = ErrorQueues::get();
        for (const auto& msg : eq.getErrors()) {
            log::warn("err: {}", msg);
        }
        for (const auto& msg : eq.getWarnings()) {
            log::warn("warn: {}", msg);
        }
    }

    void onMoreGames(CCObject*) {
        testNetworking();

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

// following listens to a frame and immediately plays it
void testFmod1() {
    auto& vm = GlobedAudioManager::get();
    vm.startRecording([&vm](const EncodedAudioFrame& frame){
        util::debugging::Benchmarker bench;

        ByteBuffer bb;
        bb.writeValue(frame);

        bb.setPosition(0);

        auto decodedFrame = bb.readValueUnique<EncodedAudioFrame>();
        VoicePlaybackManager::get().playFrameStreamed(0, *decodedFrame.get());
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
                auto encFrame = bb->readValueUnique<EncodedAudioFrame>();
                const auto& opusFrames = encFrame->extractFrames();
                
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

void testNetworking() {
    auto& nm = NetworkManager::get();

    std::string url = "http://127.0.0.1:41000";

    ///// yea this is ugly code but i need it here to remember for the future

    if (GlobedAccountManager::get().hasAuthKey()) {
        GlobedAccountManager::get().setSecretKey("hello");
        auto authcode = GlobedAccountManager::get().generateAuthCode();
        log::debug("reusing authcode: {}", authcode);

        web::AsyncWebRequest()
            .postRequest()
            .userAgent(util::net::webUserAgent())
            .fetch(url + "/totplogin?aid=1&aname=dankmeme01&code=" + authcode).text()
            .then([=](std::string response) {
                log::debug("got token for gameserver reused!!: {}", response);

                *GlobedAccountManager::get().authToken.lock() = response;
                GlobedAccountManager::get().accountId = 1;
                GlobedAccountManager::get().accountName = "dankmeme01";
                auto& nm = NetworkManager::get();
                nm.connect("127.0.0.1", 41001);

            }).expect([=](std::string err) { log::error("reuse /totplogin fail: {}", err); }).send();
    } else {
        web::AsyncWebRequest()
            .userAgent(util::net::webUserAgent())
            .fetch(url + "/version").text()
            .then([=](auto response) {
                geode::log::debug("got version: {}", response);

                if (std::stoi(response) == NetworkManager::PROTOCOL_VERSION) {
                    web::AsyncWebRequest()
                        .postRequest()
                        .userAgent(util::net::webUserAgent())
                        .fetch(url + "/challenge/new?aid=1").text()
                        .then([=](auto response) {
                            geode::log::debug("token: {}", response);
                            auto index = response.find(":"); // TODO check if npos
                            auto token = response.substr(index + 1);

                            auto tokenhash = util::crypto::simpleHash(token);
                            auto code = util::crypto::simpleTOTP(tokenhash);

                            web::AsyncWebRequest()
                                .postRequest()
                                .userAgent(util::net::webUserAgent())
                                .fetch(url + "/challenge/verify?aid=1&aname=dankmeme01&answer=" + code).text()
                                .then([=](std::string response) {
                                    log::debug("finished challenge: {}", response);
                                    auto authkey = util::crypto::base64Decode(response);
                                    auto& am = GlobedAccountManager::get();
                                    am.setSecretKey("hello");
                                    am.storeAuthKey(util::crypto::simpleHash(authkey));
                                    auto authcode = am.generateAuthCode();
                                    log::debug("generated authcode: {}", authcode);

                                    web::AsyncWebRequest()
                                        .postRequest()
                                        .userAgent(util::net::webUserAgent())
                                        .fetch(url + "/totplogin?aid=1&aname=dankmeme01&code=" + authcode).text()
                                        .then([=](std::string response) {
                                            log::debug("got token for gameserver!!: {}", response);
                                        }).expect([](std::string error) { log::warn("err fetching /totplogin: {}", error); }).send();
                                }).expect([](std::string error) { log::warn("err fetching /challenge/verify: {}", error); }).send();
                        }
                    ).expect([](std::string error) { log::warn("err fetching /challenge/new: {}", error); }).send();
                }
            }
        ).expect([](std::string error) { log::warn("err fetching /version: {}", error); }).send();
    }
}