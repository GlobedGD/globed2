#include "chat_cell.hpp"

#include "chatlist.hpp"
#include <audio/voice_playback_manager.hpp>
#include <data/packets/client/admin.hpp>
#include <data/packets/server/admin.hpp>
#include <managers/block_list.hpp>
#include <managers/settings.hpp>
#include <managers/profile_cache.hpp>
#include <hooks/gjbasegamelayer.hpp>
#include <hooks/gjgamelevel.hpp>
#include <ui/menu/admin/user_popup.hpp>
#include <ui/general/ask_input_popup.hpp>
#include <ui/general/intermediary_loading_popup.hpp>
#include <util/format.hpp>
#include <util/ui.hpp>

PlayerAccountData getAccountData(int id) {
    if (ProfileCacheManager::get().getData(id)) return ProfileCacheManager::get().getData(id).value();
    if (id == GJAccountManager::sharedState()->m_accountID) return ProfileCacheManager::get().getOwnAccountData();
    return {};
}

bool GlobedUserChatCell::init(std::string username, int accid, std::string messageText) {
    if (!CCLayerColor::init())
        return false;

    user = username;
    accountId = accid;

    auto GAM = GJAccountManager::sharedState();

    this->setOpacity(50);
    this->setContentSize(ccp(290, 32));
    this->setAnchorPoint(ccp(0, 0));

    auto menu = CCMenu::create();
    menu->setPosition(0,-10);

    auto playerBundle = CCMenu::create();

    playerBundle->setLayout(
        RowLayout::create()
        ->setGap(18)
            ->setGrowCrossAxis(true)
            ->setCrossAxisReverse(true)
            ->setAutoScale(false)
            ->setAxisAlignment(AxisAlignment::Start)
        );

    auto userTxt = CCLabelBMFont::create(username.c_str(), "goldFont.fnt");
    
    userTxt->setScale(.5);
    userTxt->setID("playername");

    auto playerIcon = SimplePlayer::create(getAccountData(accid).icons.cube);
    
    playerIcon->setScale(.475);
    playerIcon->setID("playericon");

    playerIcon->setColor(GameManager::get()->colorForIdx(getAccountData(accid).icons.color1));
    playerIcon->setSecondColor(GameManager::get()->colorForIdx(getAccountData(accid).icons.color2));
    
    playerBundle->addChild(playerIcon);
    playerBundle->addChild(userTxt);
    userTxt->setAnchorPoint(ccp(0, 0.5));
    playerBundle->updateLayout();
    static_cast<CCSprite*>(playerIcon->getChildren()->objectAtIndex(0))->setAnchorPoint(ccp(0, 0.6));

    CCMenuItemSpriteExtra* usernameButton;

    usernameButton = CCMenuItemSpriteExtra::create(
        playerBundle,
        this,
        menu_selector(GlobedUserChatCell::onUser)
    );

    usernameButton->setPosition(3, 35);
    usernameButton->setAnchorPoint(ccp(0, 0.5));
    usernameButton->setContentWidth(150);

    menu->addChild(usernameButton);
    menu->setID("chat-messages");
    this->addChild(menu);

    auto messageText = CCLabelBMFont::create(messageText.c_str(), "chatFont.fnt");

    messageText->setPosition(4, 10);
    messageText->setScale(.65);
    messageText->setAnchorPoint(ccp(0, 0.5));
    messageText->setID("message-text");

    this->addChild(messageText);

    return true;
}

void GlobedUserChatCell::onUser(CCObject* sender) {
    ProfilePage::create(accountId, GJAccountManager::sharedState()->m_accountID == accountId)->show();
}

GlobedUserChatCell* GlobedUserChatCell::create(std::string username, int aid, std::string messageText) {
    GlobedUserChatCell* pRet = new GlobedUserChatCell();
    if (pRet && pRet->init(username, aid, messageText)) {
        pRet->autorelease();
        return pRet;
    } else {
        delete pRet;
        return nullptr;
    }
}