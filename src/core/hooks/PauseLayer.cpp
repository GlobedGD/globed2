#include <globed/config.hpp>
#include <globed/core/RoomManager.hpp>
#include <globed/core/PopupManager.hpp>
#include <core/net/NetworkManagerImpl.hpp>

#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>

using namespace geode::prelude;

namespace globed {

static bool g_force = false;

struct GLOBED_MODIFY_ATTR HookedPauseLayer : geode::Modify<HookedPauseLayer, PauseLayer> {
    static void onModify(auto& self) {
        (void) self.setHookPriority("PauseLayer::onQuit", -10000);
    }

    $override
    void onQuit(CCObject* sender) {
        if (g_force) {
            PauseLayer::onQuit(sender);
            g_force = false;
            return;
        }

        auto& rm = RoomManager::get();
        if (rm.isInFollowerRoom() && !rm.isOwner()) {
            globed::quickPopup(
                "Note",
                "You are in a <cy>follower room</c>, you <cr>cannot</c> leave the level unless the room owner leaves as well. "
                "Proceeding will cause you to <cj>leave the room</c>, do you want to continue?",
                "Cancel", "Leave",
                [this](auto, bool yes) {
                    if (yes) {
                        g_force = true;
                        NetworkManagerImpl::get().sendLeaveRoom();
                        PauseLayer::onQuit(nullptr);
                    }
                }
            );
        }
    }
};

}