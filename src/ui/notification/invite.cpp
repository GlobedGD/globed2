#include "invite.hpp"

#include "panel.hpp"
#include <net/manager.hpp>
#include <data/packets/client/room.hpp>

using namespace geode::prelude;

bool GlobedInviteNotification::init(uint32_t roomID, const std::string_view password, const PlayerPreviewAccountData& player) {
    if (!CCLayer::init()) return false;

    const float width = 332.5f * 0.7f;
    const float height = 162.5f * 0.7f;

    float targetWidth = width * 0.65f;

    // bg
    const float cc9sw = width * 0.87f;
    const float cc9sh = height * 0.75f;
    const float mult = 2.f;
    Build<CCScale9Sprite>::create("GJ_square02.png")
        .contentSize({cc9sw * mult, cc9sh * mult})
        .pos(width / 2 + 10.f, height / 2 - 13.f)
        .scale(1.f / mult)
        .opacity(167)
        .zOrder(-1)
        .parent(this);

    // envelope icon
    Build<CCSprite>::createSpriteName("icon-invite.png"_spr)
        .rotation(-13.f)
        .scale(1.25f)
        .pos(width - cc9sw + 2.f, cc9sh - 2.f)
        .parent(this);

    Build<CCLabelBMFont>::create(player.name.c_str(), "goldFont.fnt")
        .limitLabelWidth(targetWidth, 0.76f, 0.3f)
        .pos(width / 2.f + 10.f, height - 39.f)
        .parent(this);

    Build<CCLabelBMFont>::create("invited you to a room", "bigFont.fnt")
        .scale(0.45f)
        .pos(width / 2.f + 10.f, height / 2.f - 4.f)
        .parent(this);

    auto* menu = Build<CCMenu>::create()
        .layout(RowLayout::create()->setGap(10.f))
        .pos(width / 2.f + 10.f, 23.f)
        .anchorPoint(0.5f, 0.5f)
        .contentSize(width * 0.7f, 40.f)
        .parent(this)
        .collect()
        ;

    Build<ButtonSprite>::create("Accept", "bigFont.fnt", "GJ_button_01.png", 0.8f)
        .intoMenuItem([this, roomID, password = std::string(password)](auto) {
            auto& nm = NetworkManager::get();
            if (nm.established()) {
                nm.send(JoinRoomPacket::create(roomID, password));
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

    this->setContentSize({width, height});
    this->setScale(0.7f);

    return true;
}

void GlobedInviteNotification::removeFromParent() {
    CCLayer::removeFromParent();

    GlobedNotificationPanel::get()->updateLayout();
}

GlobedInviteNotification* GlobedInviteNotification::create(uint32_t roomID, const std::string_view password, const PlayerPreviewAccountData& player) {
    auto ret = new GlobedInviteNotification;
    if (ret->init(roomID, password, player)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}