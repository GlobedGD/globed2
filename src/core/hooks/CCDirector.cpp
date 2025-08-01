#include <globed/config.hpp>
#include <globed/core/RoomManager.hpp>
#include <ui/menu/GlobedMenuLayer.hpp>

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

    // TODO: add settings layer and such
    return !typeinfo_cast<PlayLayer*>(layer) && !typeinfo_cast<GlobedMenuLayer*>(layer);
}

struct GLOBED_MODIFY_ATTR HookedCCDirector : geode::Modify<HookedCCDirector, CCDirector> {
    $override
    bool pushScene(CCScene* scene) {
        return block(scene) ? false : CCDirector::pushScene(scene);
    }

    $override
    bool replaceScene(CCScene* scene) {
        return block(scene) ? false : CCDirector::replaceScene(scene);
    }

    $override
    void popScene() {
        if (block()) return;
        CCDirector::popScene();
    }
};

}