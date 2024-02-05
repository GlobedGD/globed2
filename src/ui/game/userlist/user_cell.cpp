#include "user_cell.hpp"

#include "userlist.hpp"

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

    auto* menu = Build<CCMenu>::create()
        .pos(0.f, 0.f)
        .parent(this)
        .collect();

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

    this->refreshData(entry);

    return true;
}

void GlobedUserCell::refreshData(const PlayerStore::Entry& entry) {
    if (_data != entry) {
        _data = entry;
        percentageLabel->setString(fmt::format("{}%", lastPercentage).c_str());
    }

}

void GlobedUserCell::onOpenProfile(CCObject*) {
    ProfilePage::create(playerId, false)->show();
}

GlobedUserCell* GlobedUserCell::create(const PlayerStore::Entry& entry, const PlayerAccountData& data) {
    auto ret = new GlobedUserCell;
    if (ret->init(entry, data)) {
        return ret;
    }

    delete ret;
    return nullptr;
}