#include <Geode/Geode.hpp>
#include <Geode/modify/CCMouseDispatcher.hpp>

#ifndef GEODE_IS_IOS

#include <ui/settings/SettingsLayer.hpp>

using namespace geode::prelude;

namespace {
void scrollForList(ScrollLayer* scroll, float x, float y) {
    // only touch globed layers!
    auto scene = CCScene::get();
    auto children = scene->getChildrenExt();

    if (!scene || children.size() == 0) {
        return;
    }

    auto layer = children[0];

    if (!typeinfo_cast<globed::SettingsLayer*>(layer)) {
        return;
    }

    scroll->scrollWheel(x, y);
}

struct GLOBED_NOVTABLE CCMouseDispatcherH : Modify<CCMouseDispatcherH, CCMouseDispatcher> {
	bool dispatchScrollMSG(float x, float y) {
        for (CCMouseHandler* handler : CCArrayExt<CCMouseHandler*>(m_pMouseHandlers)) {
            auto* scroll = typeinfo_cast<ScrollLayer*>(handler->getDelegate());
            if (!scroll) continue;
            if (!cocos::nodeIsVisible(scroll)) continue;

            // only modify lists made by cue
            auto listNode = typeinfo_cast<cue::ListNode*>(scroll->getParent());
            if (!listNode) continue;

            scrollForList(scroll, x, y);
        }

        return CCMouseDispatcher::dispatchScrollMSG(x, y);
    }
};
}

#endif
