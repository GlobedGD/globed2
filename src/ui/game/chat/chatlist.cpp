#include "chatlist.hpp"

#include "chat_cell.hpp"
#include <audio/voice_playback_manager.hpp>
#include <hooks/gjbasegamelayer.hpp>
#include <managers/profile_cache.hpp>
#include <managers/friend_list.hpp>
#include <managers/settings.hpp>
#include <util/ui.hpp>
#include <util/misc.hpp>
#include <data/packets/all.hpp>

bool GlobedChatListPopup::setup() {
    m_noElasticity = true;

	setTitle("Chat!");
    this->m_title->setScale(.95);
	
    auto winSize = CCDirector::sharedDirector()->getWinSize();

    scroll = ScrollLayer::create(ccp(300, 150));
    scroll->setAnchorPoint(ccp(0, 0));
    scroll->ignoreAnchorPointForPosition(false);

    menu = CCMenu::create();

    menu->setPosition({this->m_title->getPositionX(),25});
    this->m_mainLayer->addChild(menu);

    inp = TextInput::create(260, "talk! :D", "chatFont.fnt");
    menu->addChild(inp);

    inp->setCommonFilter(CommonFilter::Any);
    inp->setMaxCharCount(40);
    
    CCSprite* reviewSpr = CCSprite::createWithSpriteFrameName("GJ_chatBtn_001.png");
    reviewSpr->setScale(0.75);

    reviewButton = CCMenuItemSpriteExtra::create(
        reviewSpr,
        this,
        menu_selector(GlobedChatListPopup::onChat)
    );

    menu->addChild(reviewButton);
    menu->setLayout(RowLayout::create());
    menu->setContentWidth(342);
    menu->updateLayout();
    menu->setTouchPriority(-510);

    background = cocos2d::extension::CCScale9Sprite::create("square02_small.png");
    background->setContentSize(scroll->getContentSize());
    background->setOpacity(75);
    background->setPosition(winSize / 2);
    this->addChild(background);
    background->addChild(scroll);

    scroll->m_contentLayer->removeAllChildren();

    this->setTouchEnabled(true);
    CCTouchDispatcher::get()->addTargetedDelegate(this, -129, true);
    CCTouchDispatcher::get()->addTargetedDelegate(scroll, -130, true);

    auto& pcm = ProfileCacheManager::get();
    auto& playerStore = GlobedGJBGL::get()->m_fields->playerStore->getAll();

    for (auto message : GlobedGJBGL::get()->m_fields->chatMessages) {
        std::string username = "N/A";

        PlayerAccountData yeah;
        if (message.first == GJAccountManager::sharedState()->m_accountID) username = GJAccountManager::sharedState()->m_username;
        if (pcm.getData(message.first)) username = pcm.getData(message.first).value().name;

        auto cell = GlobedUserChatCell::create(username, message.first, message.second);
        cell->setPositionY(nextY);
        scroll->m_contentLayer->addChild(cell);
        scroll->m_contentLayer->setAnchorPoint(ccp(0,1));
        nextY -= 35.f;
    }

    this->retain();

    return true;
}

void GlobedChatListPopup::keyBackClicked()  {
    onClose(nullptr);
}

void GlobedChatListPopup::onClose(CCObject*) {
    this->release();
    this->removeFromParent();
}

void GlobedChatListPopup::onChat(CCObject* sender) {
    auto& nm = NetworkManager::get();
    nm.send(ChatMessagePacket::create(std::string_view(inp->getString())));

    auto GAM = GJAccountManager::sharedState();

    GlobedGJBGL::get()->m_fields->chatMessages.push_back({GAM->m_accountID, inp->getString()});

    auto cell = GlobedUserChatCell::create(std::string(GAM->m_username), GAM->m_accountID, inp->getString());
    cell->setPositionY(nextY);
    scroll->m_contentLayer->addChild(cell);
    scroll->m_contentLayer->setAnchorPoint(ccp(0,1));
    nextY -= 35.f;
}

GlobedChatListPopup* GlobedChatListPopup::create() {
	auto ret = new GlobedChatListPopup();
	if (ret && ret->initAnchored(342, 240, "GJ_square01.png")) {
		ret->autorelease();
		return ret;
	}
	CC_SAFE_DELETE(ret);
	return nullptr;
}