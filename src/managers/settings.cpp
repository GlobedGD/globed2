#include "settings.hpp"

#define SKEY(cat, sstr) "_gsetting-" #cat #sstr
#define SFLAGKEY(sstr) "_gflag-" #sstr
#define STOREV(cat, sstr) \
    do { \
        constexpr static auto _skey = SKEY(cat, sstr); \
        \
        if (this->has(_skey) || ((cat.sstr) != (cat._DefaultFor##sstr))) { \
            this->store(_skey, cat.sstr); \
        } \
    } while (0) \

#define LOADV(cat, sstr) \
    this->loadOptionalInto(SKEY(cat, sstr), cat.sstr)

#define STOREF(sstr) this->store(SFLAGKEY(sstr), flags.sstr)
#define LOADF(sstr) this->loadOptionalInto(SFLAGKEY(sstr), flags.sstr)

#define RESET_SETTINGS(...) \
    do { \
        static const char* args[] = { __VA_ARGS__ }; \
        for (const char* arg : args) { \
            this->clear(arg);\
        } \
    } while (0) \

GlobedSettings::GlobedSettings() {
    this->reload();
}

void GlobedSettings::save() {
    // globed
    STOREV(globed, tpsCap);
    STOREV(globed, autoconnect);
    STOREV(globed, preloadAssets);
    STOREV(globed, fragmentationLimit);

    // overlay
    STOREV(overlay, opacity);
    STOREV(overlay, enabled);
    STOREV(overlay, hideConditionally);
    STOREV(overlay, position);

    // communication
    STOREV(communication, voiceEnabled);
    STOREV(communication, voiceProximity);
    STOREV(communication, voiceVolume);
    STOREV(communication, onlyFriends);
    STOREV(communication, lowerAudioLatency);
    STOREV(communication, audioDevice);

    // level ui
    STOREV(levelUi, progressIndicators);
    STOREV(levelUi, progressPointers);

    // players
    STOREV(players, playerOpacity);
    STOREV(players, showNames);
    STOREV(players, dualName);
    STOREV(players, nameOpacity);
    STOREV(players, statusIcons);
    STOREV(players, hideNearby);
    STOREV(players, deathEffects);
    STOREV(players, defaultDeathEffect);

    // store flags

    STOREF(seenSignupNotice);
    STOREF(seenSignupNoticev2);
    STOREF(seenVoiceChatPTTNotice);
}

void GlobedSettings::reload() {
    // globed
    LOADV(globed, tpsCap);
    LOADV(globed, autoconnect);
    LOADV(globed, preloadAssets);
    LOADV(globed, fragmentationLimit);

    // overlay
    LOADV(overlay, opacity);
    LOADV(overlay, enabled);
    LOADV(overlay, hideConditionally);
    LOADV(overlay, position);

    // communication
    LOADV(communication, voiceEnabled);
    LOADV(communication, voiceProximity);
    LOADV(communication, voiceVolume);
    LOADV(communication, onlyFriends);
    LOADV(communication, lowerAudioLatency);
    LOADV(communication, audioDevice);

    // level ui
    LOADV(levelUi, progressIndicators);
    LOADV(levelUi, progressPointers);

    // players
    LOADV(players, playerOpacity);
    LOADV(players, showNames);
    LOADV(players, dualName);
    LOADV(players, nameOpacity);
    LOADV(players, statusIcons);
    LOADV(players, hideNearby);
    LOADV(players, deathEffects);
    LOADV(players, defaultDeathEffect);

    // load flags

    LOADF(seenSignupNotice);
    LOADF(seenSignupNoticev2);
    LOADF(seenVoiceChatPTTNotice);
}

void GlobedSettings::resetToDefaults() {
    RESET_SETTINGS(
        SKEY(globed, tpsCap),
        SKEY(globed, autoconnect),
        SKEY(players, preloadAssets),
        SKEY(globed, fragmentationLimit),

        // overlay
        SKEY(overlay, opacity),
        SKEY(overlay, enabled),
        SKEY(overlay, hideConditionally),
        SKEY(overlay, position),

        // communication
        SKEY(communication, voiceEnabled),
        SKEY(communication, voiceProximity),
        SKEY(communication, voiceVolume),
        SKEY(communication, onlyFriends),
        SKEY(communication, lowerAudioLatency),
        SKEY(communication, audioDevice),

        // level ui
        SKEY(levelUi, progressIndicators),
        SKEY(levelUi, progressPointers),

        // players
        SKEY(players, playerOpacity),
        SKEY(players, showNames),
        SKEY(players, dualName),
        SKEY(players, nameOpacity),
        SKEY(players, statusIcons),
        SKEY(players, deathEffects),
        SKEY(players, defaultDeathEffect),
    );

    this->hardReset();
}

void GlobedSettings::hardReset() {
    // mat
    new (this) GlobedSettings;
}

void GlobedSettings::clear(const std::string_view key) {
    auto& container = Mod::get()->getSaveContainer();
    auto& obj = container.as_object();

    if (obj.contains(key)) {
        obj.erase(key);
    }
}
