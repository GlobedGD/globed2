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

bool GlobedUserChatCell::init(std::string username, int accid, std::string messageText) {
    if (!CCLayerColor::init())
        return false;

    user = username;
    accountId = accid;

    auto GAM = GJAccountManager::sharedState();

    this->setOpacity(50);
    this->setContentSize(ccp(280, 32));
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

    auto playerIcon = SimplePlayer::create(0);
    
    playerIcon->setScale(.475);
    playerIcon->setID("playericon");
    
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
    menu->setID("chat_messages"_spr);
    this->addChild(menu);

    auto revText = CCLabelBMFont::create(messageText.c_str(), "chatFont.fnt");

    revText->setPosition(4, 10);
    revText->setScale(.65);
    revText->setAnchorPoint(ccp(0, 0.5));

    this->addChild(revText);

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