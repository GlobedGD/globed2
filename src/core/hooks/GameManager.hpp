#pragma once
#include <core/PreloadManager.hpp>
#include <globed/config.hpp>

#include <Geode/Geode.hpp>
#include <Geode/modify/GameManager.hpp>
#include <asp/time.hpp>

namespace globed {

struct GLOBED_MODIFY_ATTR HookedGameManager : geode::Modify<HookedGameManager, GameManager> {
    $override cocos2d::CCTexture2D *loadIcon(int id, int type, int requestId);

    $override void unloadIcon(int iconId, int iconType, int idk);

    $override void returnToLastScene(GJGameLevel *level);

    static HookedGameManager &get();

    void setPopSceneEnum();
    void setNoopSceneEnum();
};

} // namespace globed
