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
};

#define MAKE_SETTING(ttag, button, name) \
    Build(CCMenuItemToggler::createWithStandardSprites(this, menu_selector(RoomSettingsPopup::onSettingClicked), 0.75f)) \
        .tag(ttag) \
        .store(button) \
        .intoNewParent(CCMenu::create()) \
        .layout(RowLayout::create()->setGap(3.f)->setAutoScale(false)) \
        .contentSize(POPUP_WIDTH, 50.f) \
        .id("setting-wrapper") \
        .parent(buttonLayout) \
        .intoNewChild(CCLabelBMFont::create(name, "bigFont.fnt")) \
        .scale(0.7f);

bool RoomSettingsPopup::setup() {
    auto popupLayout = util::ui::getPopupLayout(m_size);

    auto* buttonLayout = Build<CCNode>::create()
        .layout(ColumnLayout::create()->setGap(5.f))
        .anchorPoint(0.f, 1.f)
        .contentSize(POPUP_WIDTH, POPUP_HEIGHT)
        .pos(popupLayout.topLeft)
        .parent(m_mainLayer)
        .id("button-layout"_spr)
        .collect();

    MAKE_SETTING(TAG_COLLISION, btnCollision, "Collision");
    MAKE_SETTING(TAG_TWO_PLAYER, btnTwoPlayer, "2-player mode");

    for (auto item : CCArrayExt<CCNode*>(buttonLayout->getChildren())) {
        item->updateLayout();
    }

    buttonLayout->updateLayout();

    NetworkManager::get().addListener<RoomInfoPacket>([this](auto packet) {
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
    NetworkManager::get().removeListener<RoomInfoPacket>(util::time::seconds(3));
}

void RoomSettingsPopup::onSettingClicked(cocos2d::CCObject* sender) {
    bool enabled = !static_cast<CCMenuItemToggler*>(sender)->isOn();
    int setting = sender->getTag();

    switch (setting) {
        case TAG_COLLISION: currentSettings.collision = enabled; break;
        case TAG_TWO_PLAYER: currentSettings.twoPlayerMode = enabled; break;
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
    btnCollision->toggle(currentSettings.collision);
    btnTwoPlayer->toggle(currentSettings.twoPlayerMode);

    this->enableCheckboxes(RoomManager::get().isOwner());
}

void RoomSettingsPopup::enableCheckboxes(bool enabled) {
    log::debug("{} checkboxes", enabled ? "enabling" : "disabling");
    btnCollision->setEnabled(enabled);
    btnTwoPlayer->setEnabled(enabled);
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