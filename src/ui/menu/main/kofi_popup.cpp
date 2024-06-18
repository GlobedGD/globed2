#include "kofi_popup.hpp"

#include <util/ui.hpp>
#include <Geode/utils/web.hpp>

using namespace geode::prelude;

bool GlobedKofiPopup::setup(CCSprite* bg) {
    m_bgSprite->removeFromParent();

    auto rlayout = util::ui::getPopupLayout(m_size);

    auto winSize = CCDirector::get()->getWinSize();
    bg->setAnchorPoint({0.f, 0.f});
    m_mainLayer->addChild(bg);

    constexpr float of = 5.f;

    CCPoint rectangle[4] {
        CCPoint(of, of),
        CCPoint(of, m_size.height - of),
        CCPoint(m_size.width - of, m_size.height - of),
        CCPoint(m_size.width - of, of)
    };

    constexpr float speed = 1.5f;

    auto bgSequence = CCRepeatForever::create(CCSequence::create(
        CCTintTo::create(speed, 40, 190, 255),
        CCTintTo::create(speed, 40, 255, 133),
        CCTintTo::create(speed, 120, 255, 40),
        CCTintTo::create(speed, 255, 208, 40),
        CCTintTo::create(speed, 255, 47, 40),
        CCTintTo::create(speed, 255, 40, 241),
        CCTintTo::create(speed, 65, 40, 255),
        CCTintTo::create(speed, 40, 126, 255),
        nullptr
    ));

    constexpr int tt = 30;

    auto groundSequence = CCRepeatForever::create(CCSequence::create(
        CCTintTo::create(speed, 40 - tt, 190 - tt, 255 - tt),
        CCTintTo::create(speed, 40 - tt, 255 - tt, 133 - tt),
        CCTintTo::create(speed, 120 - tt, 255 - tt, 40 - tt),
        CCTintTo::create(speed, 255 - tt, 208 - tt, 40 - tt),
        CCTintTo::create(speed, 255 - tt, 47 - tt, 40 - tt),
        CCTintTo::create(speed, 255 - tt, 40 - tt, 241 - tt),
        CCTintTo::create(speed, 65 - tt, 40 - tt, 255 - tt),
        CCTintTo::create(speed, 40 - tt, 126 - tt, 255 - tt),
        nullptr
    ));

    constexpr float scale = 1.1f;

    auto buttonSequence = CCRepeatForever::create(CCSequence::create(
        CCEaseSineInOut::create(CCScaleBy::create(0.85f, scale)),
        CCEaseSineInOut::create(CCScaleBy::create(0.85f, 1.f / scale)),
        nullptr
    ));


    auto clippingNode = CCClippingNode::create();
    clippingNode->setZOrder(-5);
    auto mask = CCDrawNode::create();
    mask->drawPolygon(rectangle, 4, ccc4FFromccc3B({0, 0, 0}), 0, ccc4FFromccc3B({0, 0, 0}));
    clippingNode->setStencil(mask);

    auto* scrollBG = util::ui::makeRepeatingBackground("game_bg_02_001.png", {40, 125, 255}, 4.f, 0.f, util::ui::RepeatMode::X);
    scrollBG->setScale(0.6f);
    clippingNode->addChild(scrollBG);
    scrollBG->runAction(bgSequence);
    this->background = scrollBG;

    auto* scrollFloor = util::ui::makeRepeatingBackground("groundSquare_02_001.png", {40 - tt, 125 - tt, 255 - tt}, 6.f, 0.f, util::ui::RepeatMode::X);
    scrollFloor->setScale(0.6f);
    scrollFloor->setPositionY(-10.f);
    clippingNode->addChild(scrollFloor);
    scrollFloor->runAction(groundSequence);
    scrollFloor->setZOrder(6.f);
    this->ground = scrollFloor;

    ccBlendFunc blending = {GL_ONE, GL_ONE};

    auto line = CCSprite::createWithSpriteFrameName("floorLine_001.png");
    line->setPosition({m_size.width / 2, 66.f});
    line->setZOrder(5);
    line->setScale(0.9f);
    line->setBlendFunc(blending);
    m_mainLayer->addChild(line);

    auto card = CCSprite::create("kofi-promo-card.png"_spr);
    card->setAnchorPoint({1.f, 1.f});
    card->setScale(0.5f);
    card->setPosition({m_size.width - 30.f, m_size.height - 30.f});
    m_mainLayer->addChild(card);

    auto scaleUpAction = CCEaseExponentialOut::create(CCScaleTo::create(1.25f, 0.85f));
    card->runAction(scaleUpAction);

    m_mainLayer->addChild(clippingNode);

    // create kofi btn
    
    auto kofiMenu = CCMenu::create();
    kofiMenu->setPosition(m_size.width / 2, 5.f);
    kofiMenu->setAnchorPoint({0, 0});
    kofiMenu->runAction(buttonSequence);

    auto kofiSprite = ButtonSprite::create("Open Ko-fi", "goldFont.fnt", "GJ_button_03.png", 1.f);
    auto kofiBtn = CCMenuItemSpriteExtra::create(kofiSprite, this, menu_selector(GlobedKofiPopup::kofiCallback));
    kofiMenu->addChild(kofiBtn);
    m_mainLayer->addChild(kofiMenu);

    auto GM = GameManager::sharedState();

    auto player = SimplePlayer::create(0);
    player->updatePlayerFrame(GM->m_playerFrame, IconType::Cube);
    player->setColor(GM->colorForIdx(GM->m_playerColor));
    player->setSecondColor(GM->colorForIdx(GM->m_playerColor2));
    player->setGlowOutline(GM->colorForIdx(GM->m_playerGlowColor));
    player->enableCustomGlowColor(GM->colorForIdx(GM->m_playerGlowColor));

    auto emitter = CCParticleSystemQuad::create();
    auto emitterColor = GM->colorForIdx(GM->m_playerColor);
    auto dict = CCDictionary::createWithContentsOfFileThreadSafe("dragEffect.plist");
    dict->setObject(CCString::create("170"), "angle");
    dict->setObject(CCString::create("20"), "angleVariance");
    dict->setObject(CCString::create("250"), "speed");

    dict->setObject(CCString::create(fmt::format("{}", emitterColor.r)), "startColorRed");
    dict->setObject(CCString::create(fmt::format("{}", emitterColor.g)), "startColorGreen");
    dict->setObject(CCString::create(fmt::format("{}", emitterColor.b)), "startColorBlue");

    dict->setObject(CCString::create(fmt::format("{}", emitterColor.r)), "finishColorRed");
    dict->setObject(CCString::create(fmt::format("{}", emitterColor.g)), "finishColorGreen");
    dict->setObject(CCString::create(fmt::format("{}", emitterColor.b)), "finishColorBlue");

    emitter->setVisible(false);
    emitter->initWithDictionary(dict, true);
    particles = emitter;
    emitter->setAnchorPoint({0, 0});
    emitter->setPosition({-1000.f, -1000.f});
    emitter->setScale(0.9);
    player->addChild(emitter);

    if (!GM->m_playerGlow) {
        player->disableGlowOutline();
    }

    clippingNode->addChild(player);
    player->setPosition({-65.f, 81.5f});

    auto playerSequence = CCSequence::create(
        CCDelayTime::create(0.25f),
        CCCallFunc::create(this, callfunc_selector(GlobedKofiPopup::kofiEnableParticlesCallback)),
        CCEaseExponentialOut::create(CCMoveBy::create(1.25f, {150.f, 0.f})),
        nullptr
    );

    auto globedLabel = GlobedNameLabel::create(GM->m_playerName.c_str(), CCSprite::createWithSpriteFrameName("role-supporter.png"_spr), RichColor({154, 88, 255}));
    globedLabel->setPosition({0, 25});

    player->addChild(globedLabel);

    player->runAction(playerSequence);

    // Build<ButtonSprite>::create("Open Ko-fi", "goldFont.fnt", "GJ_button_03.png", 1.f)
    //     .intoMenuItem([] {
    //         geode::utils::web::openLinkInBrowser("https://ko-fi.com/globed");
    //     })
    //     .pos(m_size.width / 2, 5.f)
    //     .intoNewParent(CCMenu::create())
    //     .pos(0.f, 0.f)
    //     .parent(m_mainLayer);

    return true;
}

GlobedKofiPopup* GlobedKofiPopup::create() {
    auto bg = CCSprite::create("kofi-promo-border.png"_spr);
    bg->setScale(1.5f);
    auto csize = bg->getScaledContentSize();

    auto ret = new GlobedKofiPopup();
    if (ret->initAnchored(csize.width, csize.height, bg)) {
        ret->autorelease();
        ret->scheduleUpdate();
        return ret;
    }

    delete ret;
    return nullptr;
}

void GlobedKofiPopup::update(float dt) {
    constexpr float bgWidth = 307.2f;
    constexpr float gWidth = 307.2;

    background->setPositionX(background->getPositionX() - dt * (bgWidth * 0.14f));
    if (std::fabs(background->getPositionX()) > bgWidth * 2) {
        background->setPositionX(background->getPositionX() + bgWidth * 2);
    }

    ground->setPositionX(ground->getPositionX() - dt * (gWidth * 0.42f));
    if (std::fabs(ground->getPositionX()) > gWidth * 2) {
        ground->setPositionX(ground->getPositionX() + gWidth * 2);
    }
}

void GlobedKofiPopup::kofiCallback(CCObject* sender) {
    geode::utils::web::openLinkInBrowser("https://ko-fi.com/globed");
}

void GlobedKofiPopup::kofiEnableParticlesCallback() {
    particles->setVisible(true);
    particles->setPosition({-15.f, -15.f});
}