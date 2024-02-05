#include "user_cell.hpp"

#include "userlist.hpp"
#include <managers/block_list.hpp>
#include <hooks/play_layer.hpp>
#include <util/format.hpp>

using namespace geode::prelude;

bool GlobedUserCell::init(const PlayerStore::Entry& entry, const PlayerAccountData& data) {
    if (!CCLayer::init()) return false;

    playerId = data.id;

    // TODO finish this

    auto winSize = CCDirector::get()->getWinSize();

    auto gm = GameManager::get();

    auto sp = Build<SimplePlayer>::create(data.icons.cube)
        .playerFrame(data.icons.cube, IconType::Cube)
        .color(gm->colorForIdx(data.icons.color1))
        .secondColor(gm->colorForIdx(data.icons.color2))
        .parent(this)
        .pos(25.f, CELL_HEIGHT - 22.f)
        .id("player-icon"_spr)
        .collect();

    Build<CCMenu>::create()
        .pos(0.f, 0.f)
        .parent(this)
        .store(menu);

    auto* nameLabel = Build<CCLabelBMFont>::create(data.name.data(), "bigFont.fnt")
        .limitLabelWidth(150.f, 0.7f, 0.1f)
        .collect();

    Build<CCMenuItemSpriteExtra>::create(nameLabel, this, menu_selector(GlobedUserCell::onOpenProfile))
        // goodness
        .pos(sp->getPositionX() + nameLabel->getScaledContentSize().width / 2.f + 25.f, CELL_HEIGHT / 2.f)
        .parent(menu);

    if (data.icons.glowColor != -1) {
        sp->setGlowOutline(gm->colorForIdx(data.icons.glowColor));
    } else {
        sp->disableGlowOutline();
    }

    // percentage label
    Build<CCLabelBMFont>::create("", "goldFont.fnt")
        .pos(nameLabel->getPosition() / 2 + nameLabel->getScaledContentSize() + CCPoint{15.f, -2.f})
        .anchorPoint({0.f, 0.5f})
        .scale(0.4f)
        .parent(this)
        .id("percentage-label"_spr)
        .store(percentageLabel);

    this->makeBlockButton();

    this->refreshData(entry);

    return true;
}

void GlobedUserCell::refreshData(const PlayerStore::Entry& entry) {
    if (_data != entry) {
        _data = entry;

        bool platformer = PlayLayer::get()->m_level->isPlatformer();
        if (platformer && _data.localBest != 0) {
            percentageLabel->setString(util::format::formatPlatformerTime(_data.localBest).c_str());
        } else if (!platformer) {
            percentageLabel->setString(fmt::format("{}%", _data.localBest).c_str());
        }
    }
}

void GlobedUserCell::makeBlockButton() {
    if (blockButton) blockButton->removeFromParent();
    if (playerId == GJAccountManager::get()->m_accountID) return;

    // mute/unmute button
    auto pl = static_cast<GlobedPlayLayer*>(PlayLayer::get());
    bool isUnblocked = pl->shouldLetMessageThrough(playerId);

    Build<CCSprite>::createSpriteName(isUnblocked ? "GJ_fxOnBtn_001.png" : "GJ_fxOffBtn_001.png")
        .scale(0.8f)
        .intoMenuItem([isUnblocked, this](auto) {
            auto& bl = BlockListMangaer::get();
            isUnblocked ? bl.blacklist(this->playerId) : bl.whitelist(this->playerId);
            this->makeBlockButton();
        })
        .pos(GlobedUserListPopup::LIST_WIDTH - 25.f, CELL_HEIGHT / 2.f)
        .parent(menu)
        .id("block-button"_spr)
        .store(blockButton);
    log::debug("mbl 4");
}

void GlobedUserCell::onOpenProfile(CCObject*) {
    ProfilePage::create(playerId, playerId == GJAccountManager::get()->m_accountID)->show();
}

GlobedUserCell* GlobedUserCell::create(const PlayerStore::Entry& entry, const PlayerAccountData& data) {
    auto ret = new GlobedUserCell;
    if (ret->init(entry, data)) {
        return ret;
    }

    delete ret;
    return nullptr;
}