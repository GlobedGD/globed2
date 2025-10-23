#include <globed/config.hpp>
#include <globed/core/RoomManager.hpp>
#include <ui/menu/GlobedMenuLayer.hpp>
#include <ui/settings/SettingsLayer.hpp>
#include <modules/ui/UIModule.hpp>

#include <Geode/Geode.hpp>
#include <Geode/modify/CCDirector.hpp>


using namespace geode::prelude;

namespace globed {

static bool block(CCScene* scene = nullptr) {
    auto& rm = RoomManager::get();
    if (!rm.isInFollowerRoom() || rm.isOwner()) {
        return false;
    }

    if (!scene) {
        scene = CCScene::get();
    }

    if (auto trans = typeinfo_cast<CCTransitionScene*>(scene)) {
        scene = trans->m_pInScene;
        if (!scene) {
            return true;
        }
    }

    auto children = scene->getChildrenExt();
    if (children.size() == 0) {
        return true;
    }

    auto layer = children[0];

    return !typeinfo_cast<PlayLayer*>(layer) && !typeinfo_cast<GlobedMenuLayer*>(layer) && !typeinfo_cast<SettingsLayer*>(layer);
}

struct GLOBED_MODIFY_ATTR HookedCCDirector : Modify<HookedCCDirector, CCDirector> {
    static void onModify(auto& self) {
        GLOBED_CLAIM_HOOKS(UIModule::get(), self,
            "cocos2d::CCDirector::pushScene",
            "cocos2d::CCDirector::replaceScene",
            "cocos2d::CCDirector::popScene",
        );
    }

    $override
    bool pushScene(CCScene* scene) {
        if (block(scene)) {
            log::info("Blocking scene switch - disallowed right now!");
            return false;
        }

        return CCDirector::pushScene(scene);
    }

    $override
    bool replaceScene(CCScene* scene) {
        if (block(scene)) {
            log::info("Blocking scene switch - disallowed right now!");
            return false;
        }

        return CCDirector::replaceScene(scene);
    }

    $override
    void popScene() {
        if (block()) {
            log::info("Blocking scene switch - disallowed right now!");
            return;
        }

        CCDirector::popScene();
    }
};

}
