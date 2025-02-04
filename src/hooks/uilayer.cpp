#include <defs/geode.hpp>
#include <hooks/gjbasegamelayer.hpp>
#include <managers/keybinds.hpp>

#include <Geode/modify/UILayer.hpp>

struct GLOBED_DLL HookedUILayer : geode::Modify<HookedUILayer, UILayer> {
    static void onModify(auto& self) {
        (void) self.setHookPriority("UILayer::handleKeypress", 9999);
    }

    void handleKeypress(cocos2d::enumKeyCodes p0, bool p1) {
        auto gjbgl = GlobedGJBGL::get();
        if (gjbgl) gjbgl->m_fields->isManuallyResettingLevel = true;
        UILayer::handleKeypress(p0, p1);
        if (gjbgl) gjbgl->m_fields->isManuallyResettingLevel = false;

        if (p0 == (cocos2d::enumKeyCodes)GlobedSettings::get().communication.voiceChatKey.get()) {
            log::info("I did it!!! - {}", p1);
        }
        if (p0 == (cocos2d::enumKeyCodes)GlobedSettings::get().communication.voiceDeafenKey.get()) {
            log::info("i did it 2!!!! - {}", p1);
        }
    }
};
