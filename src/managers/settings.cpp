#include "settings.hpp"

#define SKEY(sstr) "_gsetting-" #sstr
#define SFLAGKEY(sstr) "_gflag-" #sstr
#define STOREV(cat, sstr) \
    do { \
        static auto _defaultFor##cat##sstr = defaultFor<decltype(cat.sstr)>(#sstr); \
        if ((cat.sstr) != _defaultFor##cat##sstr) { \
            this->store(SKEY(sstr), cat.sstr);\
        } else { \
            this->clear(SKEY(sstr));\
        } \
    } while (0) \

#define LOADV(cat, sstr) \
    this->loadOptionalInto(SKEY(sstr), cat.sstr)

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
    // The reason we do macro fuckery is because we don't want to save values that haven't been changed by the user,
    // aka are at their defaults. So if the defaults get changed in the future, the user will be affected by the change too.

    STOREV(globed, tpsCap);
    STOREV(globed, audioDevice);
    STOREV(globed, autoconnect);

    STOREV(communication, voiceEnabled);

    // store flags

    STOREF(seenSignupNotice);
}

void GlobedSettings::reload() {
    LOADV(globed, tpsCap);
    LOADV(globed, audioDevice);
    LOADV(globed, autoconnect);

    LOADV(communication, voiceEnabled);

    // load flags

    LOADF(seenSignupNotice);
}

void GlobedSettings::resetToDefaults() {
    RESET_SETTINGS(
        SKEY(tpsCap),
        SKEY(audioDevice),
        SKEY(autoconnect),

        SKEY(voiceEnabled)
    );

    this->reload();
}

void GlobedSettings::clear(const std::string& key) {
    auto& container = geode::Mod::get()->getSaveContainer();
    auto& obj = container.as_object();

    if (obj.contains(key)) {
        obj.erase(key);
    }
}
