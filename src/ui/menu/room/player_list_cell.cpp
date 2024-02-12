#include "player_list_cell.hpp"

#include "room_popup.hpp"
#include "download_level_popup.hpp"
#include <hooks/gjgamelevel.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;

bool PlayerListCell::init(const PlayerRoomPreviewAccountData& data) {
    if (!CCLayer::init()) return false;
    this->data = data;

    Build<CCMenu>::create()
        .pos(RoomPopup::LIST_WIDTH, CELL_HEIGHT)
        .parent(this)
        .store(menu);

    auto* label = Build<CCLabelBMFont>::create(data.name.c_str(), "bigFont.fnt")
        .limitLabelWidth(170.f, 0.8f, 0.1f)
        .collect();

    label->setScale(label->getScale() * 0.9f);

    auto* btn = Build<CCMenuItemSpriteExtra>::create(label, this, menu_selector(PlayerListCell::onOpenProfile))
        .pos(-RoomPopup::LIST_WIDTH + 57.f, -CELL_HEIGHT / 2.f)
        .parent(menu)
        .collect();

    // this achieves the same thing as setting anchor point X to 0.0f, but with the correct bounce
    btn->setPositionX(btn->getPositionX() + btn->getScaledContentSize().width / 2);

    auto* gm = GameManager::get();

    Build<SimplePlayer>::create(data.cube)
        .playerFrame(data.cube, IconType::Cube)
        .color(gm->colorForIdx(data.color1))
        .secondColor(gm->colorForIdx(data.color2))
        .parent(this)
        .pos(25.f, CELL_HEIGHT / 2.f)
        .store(simplePlayer);

    if (data.glowColor == -1) {
        simplePlayer->disableGlowOutline();
    } else {
        simplePlayer->setGlowOutline(gm->colorForIdx(data.glowColor));
    }

    if (this->data.levelId != 0) {
        Build<CCSprite>::createSpriteName("GJ_playBtn2_001.png")
            .scale(0.4f)
            .intoMenuItem([levelId = this->data.levelId](auto) {
                auto* glm = GameLevelManager::sharedState();
                auto mlevel = glm->m_mainLevels->objectForKey(std::to_string(levelId));

                // if it's a main level, just create a playlayer
                if (mlevel != nullptr) {
                    auto level = static_cast<HookedGJGameLevel*>(glm->getMainLevel(levelId, false));
                    level->m_fields->shouldTransitionWithPopScene = true;
                    auto pl = PlayLayer::create(level, false, false);
                    util::ui::switchToScene(pl);
                    return;
                }

                if (auto popup = DownloadLevelPopup::create(levelId)) {
                    popup->show();
                }
            })
            .pos(-30.f, -CELL_HEIGHT / 2.f)
            .parent(menu);
    }

    return true;
}

void PlayerListCell::onOpenProfile(cocos2d::CCObject*) {
    GameLevelManager::sharedState()->storeUserName(data.userId, data.id, data.name);
    ProfilePage::create(data.id, false)->show();
}

PlayerListCell* PlayerListCell::create(const PlayerRoomPreviewAccountData& data) {
    auto ret = new PlayerListCell;
    if (ret->init(data)) {
        return ret;
    }

    delete ret;
    return nullptr;
}