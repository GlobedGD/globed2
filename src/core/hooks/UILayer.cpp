#include "GJBaseGameLayer.hpp"
#include <globed/core/KeybindsManager.hpp>

#include <Geode/modify/UILayer.hpp>

using namespace geode::prelude;

namespace globed {

struct GLOBED_MODIFY_ATTR HookedUILayer : geode::Modify<HookedUILayer, UILayer> {
    static void onModify(auto &self)
    {
        (void)self.setHookPriority("UILayer::handleKeypress", 999);
    }

    void handleKeypress(enumKeyCodes p0, bool down)
    {
        auto gjbgl = GlobedGJBGL::get();

        if (p0 != KEY_None && p0 != KEY_Unknown) {
            auto &km = KeybindsManager::get();
            down ? km.handleKeyDown(p0) : km.handleKeyUp(p0);
        }

        if (gjbgl)
            gjbgl->m_fields->m_manualReset = true;

        UILayer::handleKeypress(p0, down);

        if (gjbgl)
            gjbgl->m_fields->m_manualReset = false;
    }
};

} // namespace globed
