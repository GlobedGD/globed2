#include <defs/all.hpp>

#if GLOBED_HAS_KEYBINDS
# include <geode.custom-keybinds/include/Keybinds.hpp>
#endif

#include <asp/Log.hpp>
#include <asp/async/Runtime.hpp>

#include <hooks/all.hpp>
#include <audio/manager.hpp>
#include <crypto/box.hpp>
#include <managers/settings.hpp>
#include <ui/error_check_node.hpp>
#include <ui/notification/panel.hpp>
#include <util/all.hpp>

using namespace geode::prelude;

void setupErrorCheckNode();
void setupCustomKeybinds();
void printDebugInfo();

#ifdef GLOBED_VOICE_SUPPORT
// TODO this hook kinda doesnt work bc early load and i dont even know if it would do anything
static void FMODSystemInitHook(FMOD::System* system, int channels, FMOD_INITFLAGS flags, void* dd) {
    log::debug("fmod system init hooked, changing to {} channels", MAX_AUDIO_CHANNELS);
    system->init(MAX_AUDIO_CHANNELS, flags, dd);
}
#endif // GLOBED_VOICE_SUPPORT

$execute {
    using namespace asp;

    asp::setLogFunction([](LogLevel level, auto message) {
        switch (level) {
            case LogLevel::Trace: [[fallthrough]];
            case LogLevel::Debug: log::debug("[asp] {}", message); break;
            case LogLevel::Info: log::info("[asp] {}", message); break;
            case LogLevel::Warn: log::warn("[asp] {}", message); break;
            case LogLevel::Error: log::error("[asp] {}", message); break;
        }
    });

    // auto& rt = asp::async::Runtime::get();
    // rt.launch();
}

$on_mod(Loaded) {
#ifdef GLOBED_VOICE_SUPPORT
    // (void) Mod::get()->hook(
    //     reinterpret_cast<void*>(
    //         geode::addresser::getNonVirtual(
    //             &FMOD::System::init
    //         )
    //     ),
    //     &FMODSystemInitHook,
    //     "FMOD::System::init",
    //     tulip::hook::TulipConvention::Stdcall
    // ).expect("failed to hook fmod").unwrap();
#endif // GLOBED_VOICE_SUPPORT

    CryptoBox::initLibrary();
    setupErrorCheckNode();
    setupCustomKeybinds();

#ifdef GLOBED_VOICE_SUPPORT
    GlobedAudioManager::get().preInitialize();
#endif

#if defined(GLOBED_DEBUG) && GLOBED_DEBUG
    printDebugInfo();
#endif
}

// error check node runs on every scene and shows popups/notifications if an error has occured in another thread
void setupErrorCheckNode() {
    auto ecn = ErrorCheckNode::create();
    ecn->setID("error-check-node"_spr);
    SceneManager::get()->keepAcrossScenes(ecn);

    auto notif = GlobedNotificationPanel::create();
    notif->setID("notification-panel"_spr);
    notif->persist();
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
    unsigned int fv = 0;

#if GLOBED_HAS_FMOD
    FMODAudioEngine::sharedEngine()->m_system->getVersion(&fv);
#endif

    log::warn("=== Globed {} has been loaded in debug mode ===", version.starts_with('v') ? version : ("v" + version));
    log::info("Platform: {} ({}-endian)", GLOBED_PLATFORM_STRING, GLOBED_LITTLE_ENDIAN ? "little" : "big");
    log::info("FMOD linkage: {}, version: {:X}", GLOBED_HAS_FMOD == 0 ? "false" : "true", fv);
#ifdef GLOBED_VOICE_SUPPORT
    log::info("Voice chat support: true (opus version: {})", GlobedAudioManager::getOpusVersion());
#else
    log::info("Voice chat support: false");
#endif
    log::info("Discord RPC support: {}", GLOBED_HAS_DRPC == 0 ? "false" : "true");
    log::info("Libsodium version: {} (CryptoBox algorithm: {})", CryptoBox::sodiumVersion(), CryptoBox::algorithm());
}
