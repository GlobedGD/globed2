#include "InviteNotification.hpp"
#include "NotificationPanel.hpp"
#include <globed/core/actions.hpp>
#include <core/net/NetworkManagerImpl.hpp>

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

bool InviteNotification::init(const msg::InvitedMessage& msg) {

    if (!CCLayer::init()) return false;

    this->setID(fmt::format("room-invite-{}", msg.token));

    const float width = 332.5f * 0.7f;
    const float height = 162.5f * 0.7f;

    float targetWidth = width * 0.65f;

    // bg
    const float cc9sw = width * 0.87f;
    const float cc9sh = height * 0.75f;
    const float mult = 2.f;
    Build<CCScale9Sprite>::create("GJ_square02.png")
        .id("background")
        .contentSize({cc9sw * mult, cc9sh * mult})
        .pos(width / 2 + 10.f, height / 2 - 3.f)
        .scale(1.f / mult)
        .opacity(167)
        .zOrder(-1)
        .parent(this);

    // envelope icon
    Build<CCSprite>::create("icon-invite.png"_spr)
        .id("envelope-icon")
        .rotation(-13.f)
        .scale(1.25f)
        .pos(width - cc9sw + 2.f, cc9sh + 8.f)
        .parent(this);

    Build<CCLabelBMFont>::create(msg.invitedBy.username.c_str(), "goldFont.fnt")
        .id("name-label")
        .limitLabelWidth(targetWidth, 0.76f, 0.3f)
        .intoMenuItem([player = msg.invitedBy] {
            globed::openUserProfile(player);
        })
        .id("name-btn")
        .pos(width / 2.f + 10.f, height - 29.f)
        .intoNewParent(CCMenu::create())
        .id("name-menu")
        .with([&](auto* menu) {
            menu->setTouchPriority(-1000); // this is stupid
        })
        .pos(0.f, 0.f)
        .parent(this);

    Build<CCLabelBMFont>::create("invited you to a room", "bigFont.fnt")
        .id("hint")
        .scale(0.45f)
        .pos(width / 2.f + 10.f, height / 2.f + 6.f)
        .parent(this);

    auto* menu = Build<CCMenu>::create()
        .id("button-menu")
        .layout(RowLayout::create()->setGap(10.f))
        .pos(width / 2.f + 10.f, 33.f)
        .anchorPoint(0.5f, 0.5f)
        .contentSize(width * 0.7f, 40.f)
        .parent(this)
        .collect()
        ;

    menu->setTouchPriority(-1000); // this is stupid x2

    Build<ButtonSprite>::create("Accept", "bigFont.fnt", "GJ_button_01.png", 0.8f)
        .intoMenuItem([this, token = msg.token](auto) {
            auto& nm = NetworkManagerImpl::get();
            nm.sendJoinRoomByToken(token);
            this->removeFromParent();
        })
        .id("btn-accept")
        .parent(menu)
        ;

    Build<ButtonSprite>::create("Reject", "bigFont.fnt", "GJ_button_06.png", 0.8f)
        .intoMenuItem([this](auto) {
            this->removeFromParent();
        })
        .id("btn-reject")
        .parent(menu)
        ;

    menu->updateLayout();

    this->setContentSize({width, height});
    this->setScale(0.7f);

    return true;
}

void InviteNotification::removeFromParent() {
    CCLayer::removeFromParent();

    NotificationPanel::get()->onNotificationRemoved();
}

InviteNotification* InviteNotification::create(const msg::InvitedMessage& msg) {
    auto ret = new InviteNotification;
    if (ret->init(msg)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

}