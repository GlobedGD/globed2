#include "level_area_inner_layer.hpp"

#include <data/packets/client/general.hpp>
#include <data/packets/server/general.hpp>
#include <net/manager.hpp>

using namespace geode::prelude;

bool HookedLevelAreaInnerLayer::init(bool p0) {
    if (!LevelAreaInnerLayer::init(p0)) return false;

    auto& nm = NetworkManager::get();
    if (!nm.established()) return true;

    // soft node ids dependency
    if (!Loader::get()->isModLoaded("geode.node-ids")) return true;

    auto* mainMenu = this->getChildByIDRecursive("main-menu");
    if (!mainMenu) return true;

    for (int id : TOWER_LEVELS) {
        auto* door = mainMenu->getChildByID(fmt::format("level-{}-button", id));
        if (!door) continue;

        auto* doorSprite = getChildOfType<CCSprite>(door, 0);

        const float scale = 0.45f;

        auto* wrapper = Build<CCNode>::create()
            .pos(0.f, 12.f)
            .scale(scale)
            .contentSize(doorSprite->getScaledContentSize().width / scale, 0.f)
            .layout(RowLayout::create()->setGap(5.f))
            .parent(door)
            .id("door-playercount-wrapper"_spr)
            .collect();

        Build<CCLabelBMFont>::create("", "goldFont.fnt")
            .id("door-playercount-label"_spr)
            .parent(wrapper);

        Build<CCSprite>::createSpriteName("icon-person.png"_spr)
            .id("door-playercount-icon"_spr)
            .parent(wrapper);

        wrapper->updateLayout();

        m_fields->doorNodes[id] = wrapper;
    }

    nm.addListener<LevelPlayerCountPacket>(this, [this](auto packet) {
        auto currentLayer = getChildOfType<LevelAreaInnerLayer>(CCScene::get(), 0);
        if (currentLayer && this != currentLayer) return;

        m_fields->levels.clear();
        for (const auto& level : packet->levels) {
            m_fields->levels[level.first] = level.second;
        }

        this->updatePlayerCounts();
    });

    this->schedule(schedule_selector(HookedLevelAreaInnerLayer::sendRequest), 5.f);
    this->sendRequest(0.f);

    return true;
}

void HookedLevelAreaInnerLayer::sendRequest(float) {
    auto& nm = NetworkManager::get();
    if (nm.established()) {
        nm.send(RequestPlayerCountPacket::create(std::move(std::vector<LevelId>(TOWER_LEVELS.begin(), TOWER_LEVELS.end()))));
    }
}

void HookedLevelAreaInnerLayer::updatePlayerCounts() {
    for (int id : TOWER_LEVELS) {
        if (!m_fields->levels.contains(id)) {
            m_fields->doorNodes[id]->setVisible(false);
            continue;
        }

        if (m_fields->levels.at(id) == 0) {
            m_fields->doorNodes[id]->setVisible(false);
            continue;
        }

        m_fields->doorNodes[id]->setVisible(true);
        auto* label = m_fields->doorNodes[id]->getChildByID("door-playercount-label"_spr);
        if (!label) continue;

        static_cast<CCLabelBMFont*>(label)->setString(std::to_string(m_fields->levels.at(id)).c_str());

        m_fields->doorNodes[id]->updateLayout();
    }
}

void HookedLevelAreaInnerLayer::onBack(cocos2d::CCObject* s) {
    LevelAreaInnerLayer::onBack(s);
}

void HookedLevelAreaInnerLayer::onDoor(cocos2d::CCObject* s) {
    LevelAreaInnerLayer::onDoor(s);
}