#include "settings_layer.hpp"

#include <util/ui.hpp>
#include <managers/settings.hpp>

using namespace geode::prelude;

bool GlobedSettingsLayer::init() {
    if (!CCLayer::init()) return false;

    auto winsize = CCDirector::get()->getWinSize();

    auto listview = Build<ListView>::create(createSettingsCells(), GlobedSettingCell::CELL_HEIGHT, LIST_WIDTH, LIST_HEIGHT)
        .collect();

    Build<GJListLayer>::create(listview, "Settings", ccc4(0, 0, 0, 180), LIST_WIDTH, 220.f, 0)
        .zOrder(2)
        .anchorPoint(0.f, 0.f)
        .parent(this)
        .id("setting-list"_spr)
        .store(listLayer);

    listLayer->setPosition({winsize / 2 - listLayer->getScaledContentSize() / 2});

    util::ui::prepareLayer(this);

    return true;
}

void GlobedSettingsLayer::keyBackClicked() {
    util::ui::navigateBack();
}

#define MAKE_SETTING(cat, name, setname, setdesc) cells->addObject(GlobedSettingCell::create(&settings.cat.name, getCellType<decltype(settings.cat.name)>(), setname, setdesc, {}))
#define MAKE_SETTING_TYPE(cat, name, type, setname, setdesc) cells->addObject(GlobedSettingCell::create(&settings.cat.name, type, setname, setdesc, {}))
#define MAKE_SETTING_LIM(cat, name, setname, setdesc, ...) cells->addObject(GlobedSettingCell::create(&settings.cat.name, getCellType<decltype(settings.cat.name)>(), setname, setdesc, __VA_ARGS__))
#define MAKE_HEADER(name) cells->addObject(GlobedSettingHeaderCell::create(name))

CCArray* GlobedSettingsLayer::createSettingsCells() {
    using Type = GlobedSettingCell::Type;

    auto cells = CCArray::create();

    auto& settings = GlobedSettings::get();

    MAKE_HEADER("Globed");
    MAKE_SETTING(globed, autoconnect, "Autoconnect", "Automatically connect to the last connected server on launch.");
    MAKE_SETTING_LIM(globed, tpsCap, "TPS cap", "Maximum amount of packets per second sent between the client and the server. Useful only for very silly things.", {
        .intMin = 1,
        .intMax = 240,
    });

    MAKE_HEADER("Overlay");
    MAKE_SETTING(overlay, enabled, "Ping overlay", "Show a small overlay when in a level, displaying the current latency to the server.");
    MAKE_SETTING_LIM(overlay, opacity, "Overlay opacity", "Opacity of the displayed overlay.", {
        .floatMin = 0.f,
        .floatMax = 1.f
    });
    MAKE_SETTING(overlay, hideConditionally, "Hide conditionally", "Hide the ping overlay when not connected to a server or in a non-uploaded level, instead of showing a substitute message.");

    MAKE_HEADER("Communication");
#if GLOBED_VOICE_SUPPORT
    MAKE_SETTING(communication, voiceEnabled, "Voice chat", "Enables in-game voice chat.");
    MAKE_SETTING(communication, lowerAudioLatency, "Lower audio latency", "Decreases the audio buffer size by 2 times, reducing the latency but potentially causing audio issues.");
    MAKE_SETTING_TYPE(communication, audioDevice, Type::AudioDevice, "Audio device", "The input device used for recording your voice.");
#endif // GLOBED_VOICE_SUPPORT

    return cells;
}

GlobedSettingsLayer* GlobedSettingsLayer::create() {
    auto ret = new GlobedSettingsLayer;
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}
