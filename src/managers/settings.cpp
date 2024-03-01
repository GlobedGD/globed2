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
    STOREV(communication, classicProximity);
    STOREV(communication, voiceVolume);
    STOREV(communication, onlyFriends);
    STOREV(communication, lowerAudioLatency);
    STOREV(communication, audioDevice);
    STOREV(communication, deafenNotification);

    // level ui
    STOREV(levelUi, progressIndicators);
    STOREV(levelUi, progressPointers);
    STOREV(levelUi, progressOpacity);
    STOREV(levelUi, voiceOverlay);

    // players
    STOREV(players, playerOpacity);
    STOREV(players, showNames);
    STOREV(players, dualName);
    STOREV(players, nameOpacity);
    STOREV(players, statusIcons);
    STOREV(players, hideNearby);
    STOREV(players, deathEffects);
    STOREV(players, defaultDeathEffect);
    STOREV(players, forceVisibility);

    // admin
    STOREV(admin, rememberPassword);

    // store flags

    STOREF(seenSignupNotice);
    STOREF(seenSignupNoticev2);
    STOREF(seenVoiceChatPTTNotice);
    STOREF(seenTeleportNotice);
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
    LOADV(communication, classicProximity);
    LOADV(communication, voiceVolume);
    LOADV(communication, onlyFriends);
    LOADV(communication, lowerAudioLatency);
    LOADV(communication, audioDevice);
    LOADV(communication, deafenNotification);

    // level ui
    LOADV(levelUi, progressIndicators);
    LOADV(levelUi, progressPointers);
    LOADV(levelUi, progressOpacity);
    LOADV(levelUi, voiceOverlay);

    // players
    LOADV(players, playerOpacity);
    LOADV(players, showNames);
    LOADV(players, dualName);
    LOADV(players, nameOpacity);
    LOADV(players, statusIcons);
    LOADV(players, hideNearby);
    LOADV(players, deathEffects);
    LOADV(players, defaultDeathEffect);
    LOADV(players, forceVisibility);

    // admin
    LOADV(admin, rememberPassword);

    // load flags

    LOADF(seenSignupNotice);
    LOADF(seenSignupNoticev2);
    LOADF(seenVoiceChatPTTNotice);
    LOADF(seenTeleportNotice);
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
        SKEY(communication, classicProximity),
        SKEY(communication, voiceVolume),
        SKEY(communication, onlyFriends),
        SKEY(communication, lowerAudioLatency),
        SKEY(communication, audioDevice),
        SKEY(communication, deafenNotification),

        // level ui
        SKEY(levelUi, progressIndicators),
        SKEY(levelUi, progressPointers),
        SKEY(levelUi, progressOpacity),
        SKEY(levelUi, voiceOverlay),

        // players
        SKEY(players, playerOpacity),
        SKEY(players, showNames),
        SKEY(players, dualName),
        SKEY(players, nameOpacity),
        SKEY(players, statusIcons),
        SKEY(players, deathEffects),
        SKEY(players, defaultDeathEffect),
        SKEY(players, forceVisibility),

        // admin
        SKEY(admin, rememberPassword)
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
