#include "user_cell.hpp"

#include "userlist.hpp"

using namespace geode::prelude;

bool GlobedUserCell::init(const PlayerStore::Entry& entry, const std::string_view name) {
    if (!CCLayer::init()) return false;

    // TODO finish this

    auto winSize = CCDirector::get()->getWinSize();

    auto* menu = Build<CCMenu>::create()
        .pos(0.f, 0.f)
        .parent(this)
        .collect();

    auto* label = Build<CCLabelBMFont>::create(name.data(), "bigFont.fnt")
        .collect();

    Build<CCMenuItemSpriteExtra>::create(label, this, menu_selector(GlobedUserCell::onOpenProfile))
        .pos(GlobedUserListPopup::LIST_WIDTH - 20.f, CELL_HEIGHT / 2.f)
        .parent(menu);

    this->refreshData(entry);

    return true;
}

void GlobedUserCell::refreshData(const PlayerStore::Entry& entry) {
    _data = entry;
    // TODO actually finish this
}

void GlobedUserCell::onOpenProfile(CCObject*) {
    ProfilePage::create(playerId, false)->show();
}

GlobedUserCell* GlobedUserCell::create(const PlayerStore::Entry& entry, const std::string_view name) {
    auto ret = new GlobedUserCell;
    if (ret->init(entry, name)) {
        return ret;
    }

    delete ret;
    return nullptr;
}