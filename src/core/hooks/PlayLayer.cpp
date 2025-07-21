#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <core/CoreImpl.hpp>
#include <core/net/NetworkManagerImpl.hpp>
#include <globed/core/RoomManager.hpp>
#include <globed/config.hpp>

using namespace geode::prelude;

namespace globed {

struct GLOBED_NOVTABLE HookedPlayLayer : geode::Modify<HookedPlayLayer, PlayLayer> {
    $override
    bool init(GJGameLevel* level, bool a, bool b) {
        if (!PlayLayer::init(level, a, b)) return false;

        auto& rm = RoomManager::get();
        rm.joinLevel(level->m_levelID);

        return true;
    }

    $override
    void onQuit() {
        auto& rm = RoomManager::get();
        rm.leaveLevel();

        PlayLayer::onQuit();
    }
};

}