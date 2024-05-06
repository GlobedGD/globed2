#include "invite_notification.hpp"

#include <data/packets/all.hpp>
#include <net/network_manager.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;

InviteNotification* InviteNotification::create(uint32_t roomID, const PlayerRoomPreviewAccountData& player) {
    auto ret = new InviteNotification();
    if (ret && ret->init(roomID, player)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool InviteNotification::init(uint32_t roomID, const PlayerRoomPreviewAccountData& player) {
    if (!CCLayer::init()) return false;

    auto* background = Build<CCScale9Sprite>::create("GJ_square01.png", cocos2d::CCRect { .0f, .0f, 80.0f, 80.0f, })
        .contentSize(cocos2d::CCSize { 240, 70 })
        .parent(this)
        .collect();

    menu = Build<CCMenu>::create()
        .pos({ background->getContentSize().width / 2, background->getContentSize().height / 2 })
        .parent(background);
    
    Build<CCLabelBMFont>::create(player.name.c_str(), "goldFont.fnt")
        .pos(ccp(-111, 20))
        .scale(.5f)
        .limitLabelWidth(120, 0.46f, 0.1f)
        .anchorPoint({ 0.f, 0.5f })
        .parent(this);

    Build<CCLabelBMFont>::create("has invited you to a room", "bigFont.fnt")
        .pos(ccp(-9, 19))
        .scale(.5f)
        .limitLabelWidth(120, 0.46f, 0.1f)
        .anchorPoint({ 0.f, 0.5f })
        .parent(this);

    Build<ButtonSprite>::create("Accept", "bigFont.fnt", "GJ_button_01.png",  0.8f)
        .intoMenuItem([this, roomID](auto) {
            auto& nm = NetworkManager::get();
            nm.send(JoinRoomPacket::create(roomID));
            this->removeFromParentAndCleanup(true);
        })
        .scale(.6f)
        .pos(ccp(-44, -9))
        .parent(menu);

    Build<ButtonSprite>::create("Reject", "bigFont.fnt", "GJ_button_06.png", 0.8f)
        .intoMenuItem([this, roomID](auto) {
            this->removeFromParentAndCleanup(true);
        })
        .scale(.6f)
        .pos(ccp(44, -9))
        .parent(menu);

    return true;
}