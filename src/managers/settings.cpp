#include "settings.hpp"

#include <asp/misc/traits.hpp>

using namespace geode::prelude;

GlobedSettings::LaunchArgs GlobedSettings::_launchArgs = {};

GlobedSettings::GlobedSettings() {
    this->reload();
}

void GlobedSettings::reflect(TaskType taskType) {
    using SetMd = boost::describe::describe_members<GlobedSettings, boost::describe::mod_public>;

    // iterate through all categories
    boost::mp11::mp_for_each<SetMd>([&, this](auto cd) -> void {
        using CatType = typename asp::member_ptr_to_underlying<decltype(cd.pointer)>::type;
        auto catName = cd.name;

        auto& category = this->*cd.pointer;
        bool isFlag = std::string_view(catName) == "flags";

        // iterate through all settings in the category
        using CatMd = boost::describe::describe_members<CatType, boost::describe::mod_public>;
        boost::mp11::mp_for_each<CatMd>([&, this](auto setd) -> void {
            using SetTy = typename asp::member_ptr_to_underlying<decltype(setd.pointer)>::type;
            using InnerType = SetTy::Type;
            constexpr InnerType Default = SetTy::Default;

            auto setName = setd.name;

            std::string settingKey;
            if (isFlag) {
                settingKey = fmt::format("_gflag-{}", setName);
            } else {
                settingKey = fmt::format("_gsetting-{}{}", catName, setName);
            }

            auto& setting = category.*setd.pointer;

            // now, depending on whether we are saving settings or loading them, do the appropriate thing
            switch (taskType) {
                case TaskType::SaveSettings: {
                    if (this->has(settingKey) || setting.get() != Default || isFlag) {
                        this->store(settingKey, setting.get());
                    }
                } break;
                case TaskType::LoadSettings: {
                    this->loadOptionalInto(settingKey, setting.ref());
                } break;
                case TaskType::ResetSettings: {
                    // flags cant be cleared unless hard resetting
                    if (isFlag) break;
                } [[fallthrough]];
                case TaskType::HardResetSettings: {
                    setting.set(Default);
                    this->clear(settingKey);
                } break;
            }
        });
    });
}

const GlobedSettings::LaunchArgs& GlobedSettings::launchArgs() {
    return _launchArgs;
}

void GlobedSettings::hardReset() {
    this->reflect(TaskType::HardResetSettings);
}

void GlobedSettings::reset() {
    this->reflect(TaskType::ResetSettings);
}

void GlobedSettings::reload() {
    this->reflect(TaskType::LoadSettings);

    auto lf = [](std::string_view x, bool& dest) {
        dest = Loader::get()->getLaunchFlag(x);
    };

    // load launch arguments
    lf("globed-crt-fix", _launchArgs.crtFix);
    lf("globed-verbose-curl", _launchArgs.verboseCurl);
    lf("globed-skip-preload", _launchArgs.skipPreload);
    lf("globed-debug-preload", _launchArgs.debugPreload);
    lf("globed-skip-resource-check", _launchArgs.skipResourceCheck);
    lf("globed-tracing", _launchArgs.tracing);
    lf("globed-no-ssl-verification", _launchArgs.noSslVerification);
    lf("globed-fake-server-data", _launchArgs.fakeData);

    // some of those options will do nothing unless the mod is built in debug mode
#ifndef GLOBED_DEBUG
    _launchArgs.fakeData = false;
#endif
}

void GlobedSettings::save() {
    this->reflect(TaskType::SaveSettings);
}

bool GlobedSettings::has(std::string_view key) {
    return Mod::get()->hasSavedValue(key);
}

void GlobedSettings::clear(std::string_view key) {
    auto& container = Mod::get()->getSaveContainer();
    container.erase(key);
}

// verify that all members are serialized
#include <data/bytebuffer.hpp>
static void verifySettings() {
    globed::unreachable();

    GlobedSettings* ptr = nullptr;
    ByteBuffer bb;
    bb.writeValue(*ptr);
}
