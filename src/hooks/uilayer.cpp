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

    void handleKeypress(enumKeyCodes p0, bool down) {
        auto gjbgl = GlobedGJBGL::get();

        auto& km = KeybindsManager::get();
        down ? km.handleKeyDown(p0) : km.handleKeyUp(p0);

        if (gjbgl) gjbgl->m_fields->isManuallyResettingLevel = true;

        UILayer::handleKeypress(p0, down);

        if (gjbgl) gjbgl->m_fields->isManuallyResettingLevel = false;
    }
};
