#include "invite.hpp"

#include "panel.hpp"
#include <net/network_manager.hpp>
#include <data/packets/client/general.hpp>

using namespace geode::prelude;

bool GlobedInviteNotification::init(uint32_t roomID, uint32_t roomToken, const PlayerRoomPreviewAccountData& player) {
    if (!CCLayer::init()) return false;

    auto* bg = Build<CCSprite>::createSpriteName("notif-invite-bg.png"_spr)
        .zOrder(-1)
        .scale(0.7f)
        .anchorPoint(0.f, 0.f)
        .parent(this)
        .collect()
        ;

    const auto [width, height] = bg->getScaledContentSize();
    float targetWidth = width * 0.65f;

    Build<CCLabelBMFont>::create("availax", "goldFont.fnt")
        .limitLabelWidth(targetWidth, 0.95f, 0.3f)
        .pos(width / 2.f + 10.f, height - 23.f)
        .parent(this);

    Build<CCLabelBMFont>::create("invited you", "bigFont.fnt")
        .scale(0.45f)
        .pos(width / 2.f + 10.f, height / 2.f + 15.f)
        .parent(this);

    Build<CCLabelBMFont>::create("to a room", "bigFont.fnt")
        .scale(0.45f)
        .pos(width / 2.f + 10.f, height / 2.f)
        .parent(this);

    auto* menu = Build<CCMenu>::create()
        .layout(RowLayout::create()->setGap(10.f))
        .pos(width / 2.f + 10.f, 25.f)
        .anchorPoint(0.5f, 0.5f)
        .contentSize(width * 0.7f, 40.f)
        .parent(this)
        .collect()
        ;

    Build<ButtonSprite>::create("Accept", "bigFont.fnt", "GJ_button_01.png", 0.8f)
        .intoMenuItem([this, roomID, roomToken](auto) {
            auto& nm = NetworkManager::get();
            if (nm.established()) {
                nm.send(JoinRoomPacket::create(roomID, roomToken));
            }

            this->removeFromParent();
        })
        .parent(menu)
        ;

    Build<ButtonSprite>::create("Reject", "bigFont.fnt", "GJ_button_06.png", 0.8f)
        .intoMenuItem([this, roomID](auto) {
            this->removeFromParent();
        })
        .parent(menu)
        ;

    menu->updateLayout();

    this->setContentSize(bg->getScaledContentSize());
    this->setScale(0.7f);

    return true;
}

void GlobedInviteNotification::removeFromParent() {
    CCLayer::removeFromParent();

    GlobedNotificationPanel::get()->updateLayout();
}

GlobedInviteNotification* GlobedInviteNotification::create(uint32_t roomID, uint32_t roomToken, const PlayerRoomPreviewAccountData& player) {
    auto ret = new GlobedInviteNotification;
    if (ret->init(roomID, roomToken, player)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}