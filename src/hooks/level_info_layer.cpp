#include "level_info_layer.hpp"

#include <data/packets/client/general.hpp>
#include <data/packets/server/general.hpp>
#include <net/network_manager.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;

bool HookedLevelInfoLayer::init(GJGameLevel* level, bool challenge) {
    if (!LevelInfoLayer::init(level, challenge)) return false;

    auto levelId = level->m_levelID.value();

    auto& nm = NetworkManager::get();

    if (nm.established()) {
        log::debug("sending player count id {}", levelId);
        nm.send(RequestPlayerCountPacket::create(levelId));
        nm.addListener<LevelPlayerCountPacket>([this, levelId = levelId](auto packet) {
            log::debug("got player count {}, {}", packet->levelId, packet->playerCount);
            auto players = packet->playerCount;
            if (levelId != packet->levelId) return;

            auto currentLayer = getChildOfType<LevelInfoLayer>(CCScene::get(), 0);
            if (currentLayer && this != currentLayer) return;

            this->updatePlayerCountLabel(players);
        });

        // refresh every once in a while
    }

    return true;
}

void HookedLevelInfoLayer::destructor() {
    LevelInfoLayer::~LevelInfoLayer();

    auto& nm = NetworkManager::get();
    nm.removeListener<LevelPlayerCountPacket>(util::time::seconds(3));
}

void HookedLevelInfoLayer::updatePlayerCountLabel(uint16_t playerCount) {
    if (!m_fields->playerCountLabel) {
        if (Loader::get()->isModLoaded("geode.node-ids")) {
            auto* creatorMenu = this->getChildByID("creator-info-menu");
            if (creatorMenu) {
                auto* layout = static_cast<AxisLayout*>(creatorMenu->getLayout());
                layout->setAutoScale(false);
                layout->setGap(0.f);

                Build<CCLabelBMFont>::create("", "bigFont.fnt")
                    .scale(0.4f)
                    .parent(creatorMenu)
                    .id("player-count-label"_spr)
                    .store(m_fields->playerCountLabel);

                creatorMenu->setPositionY(creatorMenu->getPositionY() - 3.f);
            }
        }

        // fallback position
        if (!m_fields->playerCountLabel) {
            auto winSize = CCDirector::get()->getWinSize();

            Build<CCLabelBMFont>::create("", "bigFont.fnt")
                .scale(0.4f)
                .pos(winSize.width / 2, winSize.height - 65.f)
                .parent(this)
                .id("player-count-label"_spr)
                .store(m_fields->playerCountLabel);
        }
    }

    if (playerCount == 69) {
        m_fields->playerCountLabel->setVisible(false);
    } else {
        m_fields->playerCountLabel->setVisible(true);
        m_fields->playerCountLabel->setString(fmt::format("{} players", playerCount).c_str());
    }

    if (Loader::get()->isModLoaded("geode.node-ids")) {
        auto* creatorMenu = this->getChildByID("creator-info-menu");
        if (creatorMenu) {
            creatorMenu->updateLayout();
        }
    }
}