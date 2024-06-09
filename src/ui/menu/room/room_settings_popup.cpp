#include "room_settings_popup.hpp"

#include <managers/error_queues.hpp>
#include <data/packets/server/room.hpp>
#include <data/packets/client/room.hpp>
#include <net/manager.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;

enum {
    TAG_COLLISION = 454,
    TAG_TWO_PLAYER,
    TAG_PUBLIC_INVITES,
    TAG_INVITE_ONLY
};

#define MAKE_SETTING(name, desc, tag, storage) \
    { \
        storage = RoomSettingCell::create(name, desc, tag, this); \
        cells->addObject(storage); \
    }

bool RoomSettingsPopup::setup() {
    auto popupLayout = util::ui::getPopupLayout(m_size);

    auto* cells = CCArray::create();

    MAKE_SETTING("Private", "While enabled, the room can not be found on the public room listing and can only be joined by entering the room ID", TAG_INVITE_ONLY, cellInviteOnly);
    MAKE_SETTING("Open invites", "While enabled, all players in the room can invite players instead of just the room owner", TAG_PUBLIC_INVITES, cellPublicInvites);
    MAKE_SETTING("Collision", "While enabled, players can collide with each other", TAG_COLLISION, cellCollision);
    MAKE_SETTING("2-player mode", "While enabled, players can link with another player to play a 2-player enabled level together", TAG_TWO_PLAYER, cellTwoPlayer);

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

void RoomSettingsPopup::onSettingClicked(cocos2d::CCObject* sender) {
    bool enabled = !static_cast<CCMenuItemToggler*>(sender)->isOn();
    int setting = sender->getTag();

    switch (setting) {
        case TAG_INVITE_ONLY: currentSettings.flags.isHidden = enabled; break;
        case TAG_PUBLIC_INVITES: currentSettings.flags.publicInvites = enabled; break;
        case TAG_COLLISION: currentSettings.flags.collision = enabled; break;
        case TAG_TWO_PLAYER: currentSettings.flags.twoPlayerMode = enabled; break;
    }

    // if we are not the room owner, just revert the changes next frame
    if (!RoomManager::get().isOwner()) {
        ErrorQueues::get().warn("Not the room creator");
        Loader::get()->queueInMainThread([self = Ref(this)] {
            self->updateCheckboxes();
        });
    } else {
        // otherwise, actually update the settings
        log::debug("settings: {}", currentSettings.flags.isHidden, currentSettings.flags.publicInvites, currentSettings.flags.collision, currentSettings.flags.twoPlayerMode);
        NetworkManager::get().send(UpdateRoomSettingsPacket::create(currentSettings));
    }
}

void RoomSettingsPopup::updateCheckboxes() {
    cellInviteOnly->setToggled(currentSettings.flags.isHidden);
    cellPublicInvites->setToggled(currentSettings.flags.publicInvites);
    cellCollision->setToggled(currentSettings.flags.collision);
    cellTwoPlayer->setToggled(currentSettings.flags.twoPlayerMode);

    this->enableCheckboxes(RoomManager::get().isOwner());
}

void RoomSettingsPopup::enableCheckboxes(bool enabled) {
    cellInviteOnly->setEnabled(enabled);
    cellPublicInvites->setEnabled(enabled);
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

bool RoomSettingCell::init(const char* name, std::string desc, int tag, RoomSettingsPopup* popup) {
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

    CCMenu* layout = Build<CCMenu>::create()
        .layout(RowLayout::create()->setGap(5.f)->setAutoScale(false)->setAxisAlignment(AxisAlignment::Start))
        .contentSize({CELL_WIDTH, CELL_HEIGHT})
        .anchorPoint(0.f, 0.5f)
        .pos(10.f, CELL_HEIGHT / 2)
        .parent(this)
        .collect()
        ;

    CCLabelBMFont* nameLabel = Build<CCLabelBMFont>::create(name, "bigFont.fnt")
        .scale(0.6f)
        //.anchorPoint(0.f, 0.5f)
        //.pos(10.f, CELL_HEIGHT / 2)
        .parent(layout)
        .collect()
        ;

    if (!desc.empty()) {
        Build<CCSprite>::createSpriteName("GJ_infoIcon_001.png")
            .scale(0.75f)
            .intoMenuItem([this, name, desc] {
                FLAlertLayer::create(
                    name,
                    desc,
                    "OK"
                )->show();
            })
            .move(ccp(0.f, -3.f))
            //.pos(10.f + (std::string(name).size() * 12.f), CELL_HEIGHT / 2)
            //.intoNewParent(CCMenu::create())
            //.pos(0.f, 0.f)
            .parent(layout)
            ;
    }

    layout->updateLayout();

    return true;
}

void RoomSettingCell::setToggled(bool state) {
    button->toggle(state);
}

void RoomSettingCell::setEnabled(bool state) {
    button->setEnabled(state);
}

RoomSettingCell* RoomSettingCell::create(const char* name, std::string desc, int tag, RoomSettingsPopup* popup) {
    auto ret = new RoomSettingCell;
    if (ret->init(name, desc, tag, popup)) {
        return ret;
    }

    delete ret;
    return nullptr;
}
