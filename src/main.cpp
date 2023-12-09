#include <defs.hpp>
#include <Geode/Geode.hpp>

#if GLOBED_HAS_KEYBINDS
#include <geode.custom-keybinds/include/Keybinds.hpp>
#endif

#include <ui/hooks/all.hpp>
#include <ui/error_check_node.hpp>
#include <util/all.hpp>

#include <cstdint>

using namespace geode::prelude;

void setupLibsodium();
void setupErrorCheckNode();
void setupCustomKeybinds();
void printDebugInfo();

// vvv this is for testing fmod
// void createStreamDetour(FMOD::System* self, const char* unused1, FMOD_MODE mode, FMOD_CREATESOUNDEXINFO* exinfo, FMOD::Sound** sound) {
//     log::debug("creating stream with mode {}", mode);
//     log::debug("exinfo cbs: {}", exinfo->cbsize);
//     log::debug("exinfo len: {}", exinfo->length);
//     log::debug("exinfo channels: {}", exinfo->numchannels);
//     log::debug("exinfo freq: {}", exinfo->defaultfrequency);
//     log::debug("exinfo fmt: {}", exinfo->format);
//     log::debug("exinfo stype: {}", exinfo->suggestedsoundtype);
//     self->createStream(unused1, mode, exinfo, sound);
// }

$on_mod(Loaded) {
    setupLibsodium();
    setupErrorCheckNode();
    setupCustomKeybinds();

#if defined(GLOBED_DEBUG) && GLOBED_DEBUG
    printDebugInfo();
#endif

    /// vvv this is for testing fmod
    // (void) Mod::get()->addHook(reinterpret_cast<void*>(
    //     geode::addresser::getNonVirtual(&FMOD::System::createStream)
    // ), &createStreamDetour, "FMOD::System::createStream", tulip::hook::TulipConvention::Default);
}


class $modify(MyMenuLayer, MenuLayer) {
    void onMoreGames(CCObject*) {
        if (NetworkManager::get().handshaken()) {
            util::debugging::PacketLogger::get().getSummary().print();
        }

        try {
            throw std::runtime_error("oopsie");
        } catch (std::exception e) {
            log::debug("caught it! {}", e.what());
        }
    }
};

void setupLibsodium() {
    // sodium_init returns 0 on success, 1 if already initialized, -1 on fail
    GLOBED_REQUIRE(sodium_init() != -1, "sodium_init failed")

    // if there is a logic error in the crypto code, this lambda will be called
    sodium_set_misuse_handler([](){
        log::error("sodium_misuse called. we are officially screwed.");
        util::debugging::suicide();
    });
}

// error check node runs on every scene and shows popups/notifications if an error has occured in another thread
void setupErrorCheckNode() {
    auto ecn = ErrorCheckNode::create();
    ecn->setID("error-check-node"_spr);
    SceneManager::get()->keepAcrossScenes(ecn);
}

void setupCustomKeybinds() {
#if GLOBED_HAS_KEYBINDS
    using namespace keybinds;

    BindManager::get()->registerBindable({
        "voice-activate"_spr,
        "Voice",
        "Records audio from your microphone and sends it off to other users on the level.",
        { Keybind::create(KEY_V, Modifier::None) },
        Category::PLAY,
    });

    BindManager::get()->registerBindable({
        "voice-deafen"_spr,
        "Deafen",
        "Mutes voices of other players when toggled.",
        { Keybind::create(KEY_B, Modifier::None) },
        Category::PLAY,
    });

#endif // GLOBED_HAS_KEYBINDS
}

// just debug printing
void printDebugInfo() {
    std::string version = Mod::get()->getVersion().toString();

    geode::log::warn("=== Globed {} has been loaded in debug mode ===", version.starts_with('v') ? version : ("v" + version));
    geode::log::info("Platform: {}", GLOBED_PLATFORM_STRING);
    geode::log::info("FMOD support: {}", GLOBED_HAS_FMOD == 0 ? "false" : "true");
    geode::log::info("Voice support: {}", GLOBED_VOICE_SUPPORT == 0 ? "false" : "true");
    geode::log::info("Discord RPC support: {}", GLOBED_HAS_DRPC == 0 ? "false" : "true");
    geode::log::info("Little endian: {}", GLOBED_LITTLE_ENDIAN ? "true" : "false");
    geode::log::info("Libsodium version: {} (CryptoBox algorithm: {})", SODIUM_VERSION_STRING, CryptoBox::ALGORITHM);
    #if GLOBED_VOICE_SUPPORT
    geode::log::info("Opus version: {}", opus_get_version_string());
    #endif
}
