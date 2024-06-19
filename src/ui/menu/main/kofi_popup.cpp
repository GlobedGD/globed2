#include "kofi_popup.hpp"

#include <Geode/utils/web.hpp>

#include <managers/profile_cache.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;

bool GlobedKofiPopup::setup(CCSprite* bg) {
    m_bgSprite->removeFromParent();

    auto rlayout = util::ui::getPopupLayoutAnchored(m_size);

    auto winSize = CCDirector::get()->getWinSize();
    bg->setAnchorPoint({0.f, 0.f});
    m_mainLayer->addChild(bg);

    constexpr float pad = 5.f;

    CCPoint rectangle[4] {
        CCPoint(rlayout.left + pad, rlayout.bottom + pad),
        CCPoint(rlayout.left + pad, rlayout.top - pad),
        CCPoint(rlayout.right - pad, rlayout.top - pad),
        CCPoint(rlayout.right - pad, rlayout.bottom + pad)
    };

    constexpr float tintDuration = 1.5f;

    auto bgSequence = CCRepeatForever::create(CCSequence::create(
        CCTintTo::create(tintDuration, 40, 190, 255),
        CCTintTo::create(tintDuration, 40, 255, 133),
        CCTintTo::create(tintDuration, 120, 255, 40),
        CCTintTo::create(tintDuration, 255, 208, 40),
        CCTintTo::create(tintDuration, 255, 47, 40),
        CCTintTo::create(tintDuration, 255, 40, 241),
        CCTintTo::create(tintDuration, 65, 40, 255),
        CCTintTo::create(tintDuration, 40, 126, 255),
        nullptr
    ));

    constexpr int tintMod = 30;

    auto groundSequence = CCRepeatForever::create(CCSequence::create(
        CCTintTo::create(tintDuration, 40 - tintMod, 190 - tintMod, 255 - tintMod),
        CCTintTo::create(tintDuration, 40 - tintMod, 255 - tintMod, 133 - tintMod),
        CCTintTo::create(tintDuration, 120 - tintMod, 255 - tintMod, 40 - tintMod),
        CCTintTo::create(tintDuration, 255 - tintMod, 208 - tintMod, 40 - tintMod),
        CCTintTo::create(tintDuration, 255 - tintMod, 47 - tintMod, 40 - tintMod),
        CCTintTo::create(tintDuration, 255 - tintMod, 40 - tintMod, 241 - tintMod),
        CCTintTo::create(tintDuration, 65 - tintMod, 40 - tintMod, 255 - tintMod),
        CCTintTo::create(tintDuration, 40 - tintMod, 126 - tintMod, 255 - tintMod),
        nullptr
    ));

    constexpr float buttonScale = 1.1f;

    auto clippingNode = Build<CCClippingNode>::create()
        .zOrder(-5)
        .parent(m_mainLayer)
        .collect();

    auto mask = CCDrawNode::create();
    mask->drawPolygon(rectangle, 4, ccc4FFromccc3B({0, 0, 0}), 0, ccc4FFromccc3B({0, 0, 0}));
    clippingNode->setStencil(mask);

    Build(util::ui::makeRepeatingBackground("game_bg_02_001.png", {40, 125, 255}, 4.f, 0.f, util::ui::RepeatMode::X))
        .id("background")
        .scale(0.6f)
        .parent(clippingNode)
        .store(this->background)
        .collect();

    this->background->runAction(bgSequence);

    Build(util::ui::makeRepeatingBackground(
        "groundSquare_02_001.png",
        {40 - tintMod, 125 - tintMod, 255 - tintMod},
        6.f, 0.f, util::ui::RepeatMode::X)
    )
        .id("ground")
        .scale(0.6f)
        .posY(-10.f)
        .parent(clippingNode)
        .zOrder(6)
        .store(this->ground)
        .collect();

    this->ground->runAction(groundSequence);

    ccBlendFunc blending = {GL_ONE, GL_ONE};

    Build<CCSprite>::createSpriteName("floorLine_001.png")
        .id("floor-line")
        .pos(rlayout.center.width, 66.f)
        .zOrder(5)
        .scale(0.9f)
        .blendFunc(blending)
        .parent(m_mainLayer);

    Build<CCSprite>::create("kofi-promo-card.png"_spr)
        .anchorPoint(1.f, 1.f)
        .scale(0.5f)
        .pos(rlayout.topRight - CCPoint{30.f, 30.f})
        .parent(m_mainLayer)
        .with([&](CCSprite* card) {
            auto scaleUpAction = CCEaseExponentialOut::create(CCScaleTo::create(1.25f, 0.85f));
            card->runAction(scaleUpAction);
        });

    // create kofi btn

    Build<ButtonSprite>::create("Open Ko-fi", "goldFont.fnt", "GJ_button_03.png", 1.f)
        .intoMenuItem([this](auto* o) {
            this->kofiCallback(o);
        })
        .intoNewParent(CCMenu::create())
        .pos(rlayout.centerBottom + CCPoint{0.f, 5.f})
        .anchorPoint(0.f, 0.f)
        .with([&](CCMenu* menu) {
            auto buttonSequence = CCRepeatForever::create(CCSequence::create(
                CCEaseSineInOut::create(CCScaleBy::create(0.85f, buttonScale)),
                CCEaseSineInOut::create(CCScaleBy::create(0.85f, 1.f / buttonScale)),
                nullptr
            ));

            menu->runAction(buttonSequence);
        })
        .parent(m_mainLayer);

    auto* gm = GameManager::sharedState();
    auto icons = ProfileCacheManager::get().getOwnData();

    GlobedSimplePlayer* player;
    Build<GlobedSimplePlayer>::create(icons)
        .anchorPoint(0.5f, 0.5f)
        .pos(-65.f, 81.5f)
        .parent(clippingNode)
        .with([&](GlobedSimplePlayer* player) {
            auto playerSequence = CCSequence::create(
                CCDelayTime::create(0.25f),
                CCCallFunc::create(this, callfunc_selector(GlobedKofiPopup::kofiEnableParticlesCallback)),
                CCEaseExponentialOut::create(CCMoveBy::create(1.25f, {150.f, 0.f})),
                nullptr
            );

            player->runAction(playerSequence);
        })
        .store(player)
        .intoNewChild(GlobedNameLabel::create(
            gm->m_playerName.c_str(),
            CCSprite::createWithSpriteFrameName("role-supporter.png"_spr),
            RichColor({154, 88, 255})
        ))
        .pos(15.f, 40.f)
        .collect();

    auto emitter = Build<CCParticleSystemQuad>::create()
        .pos(0.f, 0.f)
        .anchorPoint(0.f, 0.f)
        .scale(0.9f)
        .zOrder(-1)
        .store(particles)
        .parent(player)
        .collect();

#ifdef GEODE_IS_WINDOWS
    emitter->setOpacity(0);
#else
    emitter->setScale(0.0f);
#endif

    auto emitterColor = gm->colorForIdx(gm->m_playerColor);
    auto dict = CCDictionary::createWithContentsOfFile("dragEffect.plist");
    dict->setObject(CCString::create("170"), "angle");
    dict->setObject(CCString::create("20"), "angleVariance");
    dict->setObject(CCString::create("250"), "speed");

    dict->setObject(CCString::create(fmt::format("{}", emitterColor.r)), "startColorRed");
    dict->setObject(CCString::create(fmt::format("{}", emitterColor.g)), "startColorGreen");
    dict->setObject(CCString::create(fmt::format("{}", emitterColor.b)), "startColorBlue");

    dict->setObject(CCString::create(fmt::format("{}", emitterColor.r)), "finishColorRed");
    dict->setObject(CCString::create(fmt::format("{}", emitterColor.g)), "finishColorGreen");
    dict->setObject(CCString::create(fmt::format("{}", emitterColor.b)), "finishColorBlue");

    emitter->initWithDictionary(dict, true);

    this->scheduleUpdate();

    return true;
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

    particles->setPosition(0.f, 0.f);
}

void GlobedKofiPopup::kofiCallback(CCObject* sender) {
    geode::utils::web::openLinkInBrowser("https://ko-fi.com/globed");
}

void GlobedKofiPopup::kofiEnableParticlesCallback() {
    this->scheduleOnce(schedule_selector(GlobedKofiPopup::kofiEnableParticlesCallback2), 0.1f);
}

void GlobedKofiPopup::kofiEnableParticlesCallback2(float dt) {
#ifdef GEODE_IS_WINDOWS
    particles->setOpacity(255);
#else
    particles->setScale(0.9f);
#endif
}

GlobedKofiPopup* GlobedKofiPopup::create() {
    auto bg = CCSprite::create("kofi-promo-border.png"_spr);
    bg->setScale(1.5f);
    auto csize = bg->getScaledContentSize();

    auto ret = new GlobedKofiPopup();
    if (ret->initAnchored(csize.width, csize.height, bg)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}