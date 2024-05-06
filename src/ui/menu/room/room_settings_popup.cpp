#include "room_settings_popup.hpp"

#include <managers/error_queues.hpp>
#include <data/packets/server/general.hpp>
#include <data/packets/client/general.hpp>
#include <net/network_manager.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;

enum {
    TAG_COLLISION = 454,
    TAG_TWO_PLAYER,
    TAG_PUBLIC_INVITES
};

#define MAKE_SETTING(name, tag, storage) \
    { \
        storage = RoomSettingCell::create(name, tag, this); \
        cells->addObject(storage); \
    }

bool RoomSettingsPopup::setup() {
    auto popupLayout = util::ui::getPopupLayout(m_size);

    auto* cells = CCArray::create();
    MAKE_SETTING("Collision", TAG_COLLISION, cellCollision);
    MAKE_SETTING("2-player mode", TAG_TWO_PLAYER, cellTwoPlayer);
    MAKE_SETTING("Public invites", TAG_PUBLIC_INVITES, cellPublicInvites);

    auto listview = ListView::create(cells, RoomSettingCell::CELL_HEIGHT, LIST_WIDTH, LIST_HEIGHT);
    auto* listlayer = Build(GJCommentListLayer::create(listview, "", util::ui::BG_COLOR_BROWN, LIST_WIDTH, LIST_HEIGHT, false))
        .scale(0.65f)
        .parent(m_mainLayer)
        .collect();

    listlayer->setPosition(popupLayout.center - listlayer->getContentSize() / 2);

    NetworkManager::get().addListener<RoomInfoPacket>(this, [this](auto packet) {
        log::debug("room configuration updated");

        RoomManager::get().setInfo(packet->info);
        this->currentSettings = packet->info.settings;
        this->updateCheckboxes();
    });

    currentSettings = RoomManager::get().getInfo().settings;
    this->updateCheckboxes();

    return true;
}

RoomSettingsPopup::~RoomSettingsPopup() {
    NetworkManager::get().suppressUnhandledFor<RoomInfoPacket>(util::time::seconds(3));
}

void RoomSettingsPopup::onSettingClicked(cocos2d::CCObject* sender) {
    bool enabled = !static_cast<CCMenuItemToggler*>(sender)->isOn();
    int setting = sender->getTag();

    switch (setting) {
        case TAG_COLLISION: currentSettings.collision = enabled; break;
        case TAG_TWO_PLAYER: currentSettings.twoPlayerMode = enabled; break;
        case TAG_PUBLIC_INVITES: currentSettings.publicInvites = enabled; break;
    }

    // if we are not the room owner, just revert the changes next frame
    if (!RoomManager::get().isOwner()) {
        ErrorQueues::get().warn("Not the room creator");
        Loader::get()->queueInMainThread([self = Ref(this)] {
            self->updateCheckboxes();
        });
    } else {
        // otherwise, actually update the settings
        NetworkManager::get().send(UpdateRoomSettingsPacket::create(currentSettings));
    }
}

void RoomSettingsPopup::updateCheckboxes() {
    cellCollision->setToggled(currentSettings.collision);
    cellTwoPlayer->setToggled(currentSettings.twoPlayerMode);

    this->enableCheckboxes(RoomManager::get().isOwner());
}

void RoomSettingsPopup::enableCheckboxes(bool enabled) {
    cellCollision->setEnabled(enabled);
    cellTwoPlayer->setEnabled(enabled);
}

RoomSettingsPopup* RoomSettingsPopup::create() {
    auto ret = new RoomSettingsPopup;
    if (ret->init(POPUP_WIDTH, POPUP_HEIGHT)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

bool RoomSettingCell::init(const char* name, int tag, RoomSettingsPopup* popup) {
    if (!CCLayer::init()) return false;
    this->popup = popup;

    Build(CCMenuItemToggler::createWithStandardSprites(popup, menu_selector(RoomSettingsPopup::onSettingClicked), 0.7f)) \
        .tag(tag)
        .store(button)
        .anchorPoint(0.5f, 0.5f)
        .pos(CELL_WIDTH - 20.f, CELL_HEIGHT / 2)
        .intoNewParent(CCMenu::create())
        .pos(0.f, 0.f)
        .parent(this);

    Build<CCLabelBMFont>::create(name, "bigFont.fnt")
        .scale(0.6f)
        .anchorPoint(0.f, 0.5f)
        .pos(10.f, CELL_HEIGHT / 2)
        .parent(this)
        ;

    return true;
}

void RoomSettingCell::setToggled(bool state) {
    button->toggle(state);
}

void RoomSettingCell::setEnabled(bool state) {
    button->setEnabled(state);
}

RoomSettingCell* RoomSettingCell::create(const char* name, int tag, RoomSettingsPopup* popup) {
    auto ret = new RoomSettingCell;
    if (ret->init(name, tag, popup)) {
        return ret;
    }

    delete ret;
    return nullptr;
}
