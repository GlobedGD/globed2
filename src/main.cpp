#include <defs/all.hpp>

#if GLOBED_HAS_KEYBINDS
#include <geode.custom-keybinds/include/Keybinds.hpp>
#endif

#include <Geode/cocos/platform/IncludeCurl.h>

#include <hooks/all.hpp>
#include <audio/manager.hpp>
#include <managers/settings.hpp>
#include <ui/error_check_node.hpp>
#include <util/all.hpp>

using namespace geode::prelude;

void setupLibsodium();
void setupErrorCheckNode();
void setupCustomKeybinds();
void printDebugInfo();

#if GLOBED_VOICE_SUPPORT
static void FMODSystemInitHook(FMOD::System* system, int channels, FMOD_INITFLAGS flags, void* dd) {
    log::debug("fmod system init hooked, changing to {} channels", MAX_AUDIO_CHANNELS);
    system->init(MAX_AUDIO_CHANNELS, flags, dd);
}
#endif // GLOBED_VOICE_SUPPORT

$on_mod(Loaded) {
#if GLOBED_VOICE_SUPPORT
    (void) Mod::get()->hook(
        reinterpret_cast<void*>(
            geode::addresser::getNonVirtual(
                &FMOD::System::init
            )
        ),
        &FMODSystemInitHook,
        "FMOD::System::init",
        tulip::hook::TulipConvention::Stdcall
    ).expect("failed to hook fmod").unwrap();
#endif // GLOBED_VOICE_SUPPORT

    setupLibsodium();
    setupErrorCheckNode();
    setupCustomKeybinds();

#if GLOBED_VOICE_SUPPORT
    GlobedAudioManager::get().preInitialize();
#endif

#if defined(GLOBED_DEBUG) && GLOBED_DEBUG
    printDebugInfo();
#endif
}

void setupLibsodium() {
    // sodium_init returns 0 on success, 1 if already initialized, -1 on fail
    GLOBED_REQUIRE(sodium_init() != -1, "sodium_init failed")

    // if there is a logic error in the crypto code, this lambda will be called
    sodium_set_misuse_handler([] {
        log::error("sodium_misuse called. we are officially screwed.");
        util::debug::suicide();
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

    log::warn("=== Globed {} has been loaded in debug mode ===", version.starts_with('v') ? version : ("v" + version));
    log::info("Platform: {} ({}-endian)", GLOBED_PLATFORM_STRING, GLOBED_LITTLE_ENDIAN ? "little" : "big");
    log::info("FMOD linkage: {}", GLOBED_HAS_FMOD == 0 ? "false" : "true");
#if GLOBED_VOICE_SUPPORT
    log::info("Voice chat support: true (opus version: {})", GlobedAudioManager::getOpusVersion());
#else
    log::info("Voice chat support: false");
#endif
    log::info("Discord RPC support: {}", GLOBED_HAS_DRPC == 0 ? "false" : "true");
    log::info("Libsodium version: {} (CryptoBox algorithm: {})", SODIUM_VERSION_STRING, CryptoBox::ALGORITHM);

    auto cvi = curl_version_info(CURLVERSION_NOW);

    log::info("cURL version: {}, {}", curl_version(), (cvi->features & CURL_VERSION_SSL) ? "with SSL" : "without SSL (!)");
}
