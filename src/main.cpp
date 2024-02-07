#include <defs.hpp>
#include <Geode/Geode.hpp>

#if GLOBED_HAS_KEYBINDS
#include <geode.custom-keybinds/include/Keybinds.hpp>
#endif

#include <Geode/cocos/platform/IncludeCurl.h>

#include <audio/manager.hpp>
#include <hooks/all.hpp>
#include <ui/error_check_node.hpp>
#include <util/all.hpp>
#include <game/lerp_logger.hpp>

using namespace geode::prelude;

void setupLibsodium();
void setupErrorCheckNode();
void setupCustomKeybinds();
void loadDeathEffects();
void printDebugInfo();

$on_mod(Loaded) {
    setupLibsodium();
    setupErrorCheckNode();
    setupCustomKeybinds();
    loadDeathEffects();

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

void loadDeathEffects() {
    log::debug("Loading death effects..");
    // --geode:globed-skip-death-effects=true
    if (Loader::get()->getLaunchFlag("globed-skip-death-effects")) {
        log::info("Skipped death effect loading due to a launch flag.");
        return;
    }

    for (int i = 1; i < 21; i++) {
        log::debug("Loading death effect {}", i);
        try {
            GameManager::get()->loadDeathEffect(i);
        } catch (const std::exception& e) {
            log::warn("Failed to load death effect: {}", e.what());
        }
    }
    log::debug("Finished loading death effects");
}

// just debug printing
void printDebugInfo() {
    std::string version = Mod::get()->getVersion().toString();

    log::warn("=== Globed {} has been loaded in debug mode ===", version.starts_with('v') ? version : ("v" + version));
    log::info("Platform: {} ({}-endian)", GLOBED_PLATFORM_STRING, GLOBED_LITTLE_ENDIAN ? "little" : "big");
    log::info("FMOD linkage: {}", GLOBED_HAS_FMOD == 0 ? "false" : "true");
#if GLOBED_VOICE_SUPPORT
    log::info("Voice chat support: true (opus version: {})", opus_get_version_string());
#else
    log::info("Voice chat support: false");
#endif
    log::info("Discord RPC support: {}", GLOBED_HAS_DRPC == 0 ? "false" : "true");
    log::info("Libsodium version: {} (CryptoBox algorithm: {})", SODIUM_VERSION_STRING, CryptoBox::ALGORITHM);

    auto cvi = curl_version_info(CURLVERSION_NOW);

    log::info("cURL version: {}, {}", curl_version(), (cvi->features & CURL_VERSION_SSL) ? "with SSL" : "without SSL (!)");
}
