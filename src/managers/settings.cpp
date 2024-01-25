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

    // overlay
    STOREV(overlay, opacity);
    STOREV(overlay, enabled);
    STOREV(overlay, hideConditionally);

    // communication
    STOREV(communication, voiceEnabled);
    STOREV(communication, lowerAudioLatency);
    STOREV(communication, audioDevice);

    // store flags

    STOREF(seenSignupNotice);
}

void GlobedSettings::reload() {
    // globed
    LOADV(globed, tpsCap);
    LOADV(globed, autoconnect);

    // overlay
    LOADV(overlay, opacity);
    LOADV(overlay, enabled);
    LOADV(overlay, hideConditionally);

    // communication
    LOADV(communication, voiceEnabled);
    LOADV(communication, lowerAudioLatency);
    LOADV(communication, audioDevice);

    // load flags

    LOADF(seenSignupNotice);
}

void GlobedSettings::resetToDefaults() {
    RESET_SETTINGS(
        SKEY(globed, tpsCap),
        SKEY(globed, autoconnect),

        // overlay
        SKEY(overlay, opacity),
        SKEY(overlay, enabled),
        SKEY(overlay, hideConditionally),

        // communication
        SKEY(communication, voiceEnabled),
        SKEY(communication, lowerAudioLatency),
        SKEY(communication, audioDevice),
    );

    this->reload();
}

void GlobedSettings::clear(const std::string_view key) {
    auto& container = geode::Mod::get()->getSaveContainer();
    auto& obj = container.as_object();

    if (obj.contains(key)) {
        obj.erase(key);
    }
}
