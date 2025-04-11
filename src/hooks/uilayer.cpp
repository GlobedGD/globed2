#include <audio/voice_playback_manager.hpp>
#include <audio/manager.hpp>
#include <defs/geode.hpp>
#include <defs/platform.hpp>
#include <hooks/gjbasegamelayer.hpp>
#include <managers/keybinds.hpp>

#include <Geode/modify/UILayer.hpp>

using namespace geode::prelude;

struct GLOBED_DLL HookedUILayer : geode::Modify<HookedUILayer, UILayer> {
    static void onModify(auto& self) {
        (void) self.setHookPriority("UILayer::handleKeypress", 9999);
    }

    void handleKeypress(enumKeyCodes p0, bool p1) {
        auto gjbgl = GlobedGJBGL::get();

        if (gjbgl) gjbgl->m_fields->isManuallyResettingLevel = true;
        UILayer::handleKeypress(p0, p1);
        if (gjbgl) gjbgl->m_fields->isManuallyResettingLevel = false;
    }

    void keyDown(enumKeyCodes p0) {
        KeybindsManager::get().handleKeyDown(p0);

        UILayer::keyDown(p0);
    }

    void keyUp(enumKeyCodes p0) {
        KeybindsManager::get().handleKeyUp(p0);

        UILayer::keyUp(p0);
    }
};
