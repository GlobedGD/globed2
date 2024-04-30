#include "chatlist.hpp"
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

    for (auto message : GlobedGJBGL::get()->m_fields->chatMessages) {
        createMessage(message.first, message.second);
    }

    this->retain();
    this->schedule(schedule_selector(GlobedChatListPopup::updateChat));

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
    createMessage(GAM->m_accountID, inp->getString());
}

void GlobedChatListPopup::createMessage(int accountID, std::string message) {
    auto& pcm = ProfileCacheManager::get();
    auto& playerStore = GlobedGJBGL::get()->m_fields->playerStore->getAll();

    std::string username = "N/A";

    PlayerAccountData yeah;
    // if account ID is ours, then display our username
    if (accountID == GJAccountManager::sharedState()->m_accountID) username = GJAccountManager::sharedState()->m_username;
    // if account ID is in the player cache, get the username from there
    if (pcm.getData(accountID)) username = pcm.getData(accountID).value().name;

    auto cell = GlobedUserChatCell::create(username, accountID, message);
    cell->setPositionY(0.f);
    scroll->m_contentLayer->addChild(cell);
    scroll->m_contentLayer->setAnchorPoint(ccp(0,1));

    for (GlobedUserChatCell* gucci : messageCells) {
        gucci->setPositionY(gucci->getPositionY() + 35.f);
    }
    messageCells.push_back(cell);

    float height = std::max<float>(scroll->getContentSize().height, 35 * scroll->m_contentLayer->getChildrenCount());
    scroll->m_contentLayer->setContentSize(ccp(scroll->m_contentLayer->getContentSize().width, height));

    //scroll->moveToBottom();
}

void GlobedChatListPopup::updateChat(float dt) {
    if (GlobedGJBGL::get()->m_fields->chatMessages.size() > messageCells.size()) createMessage(GlobedGJBGL::get()->m_fields->chatMessages.back().first, GlobedGJBGL::get()->m_fields->chatMessages.back().second);
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