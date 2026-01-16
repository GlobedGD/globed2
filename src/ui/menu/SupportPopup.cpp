#include "SupportPopup.hpp"
#include <globed/util/gd.hpp>
#include <ui/misc/Badges.hpp>
#include <ui/misc/NameLabel.hpp>

#include <UIBuilder.hpp>
#include <cue/PlayerIcon.hpp>
#include <cue/RepeatingBackground.hpp>

using namespace geode::prelude;

namespace globed {

CCSize SupportPopup::POPUP_SIZE{};

bool SupportPopup::setup(CCSprite *bg)
{
    this->setID("GlobedKofiPopup"_spr);

    m_bgSprite->removeFromParent();

    auto winSize = CCDirector::get()->getWinSize();
    bg->setAnchorPoint({0.f, 0.f});
    m_mainLayer->addChild(bg);

    constexpr float pad = 5.f;

    CCPoint rectangle[4]{
        this->fromBottomLeft(pad, pad),
        this->fromTopLeft(pad, pad),
        this->fromTopRight(pad, pad),
        this->fromBottomRight(pad, pad),
    };

    constexpr float tintDuration = 1.5f;

    auto bgSequence = CCRepeatForever::create(CCSequence::create(
        CCTintTo::create(tintDuration, 40, 190, 255), CCTintTo::create(tintDuration, 40, 255, 133),
        CCTintTo::create(tintDuration, 120, 255, 40), CCTintTo::create(tintDuration, 255, 208, 40),
        CCTintTo::create(tintDuration, 255, 47, 40), CCTintTo::create(tintDuration, 255, 40, 241),
        CCTintTo::create(tintDuration, 65, 40, 255), CCTintTo::create(tintDuration, 40, 126, 255), nullptr));

    constexpr int tintMod = 30;

    auto groundSequence = CCRepeatForever::create(
        CCSequence::create(CCTintTo::create(tintDuration, 40 - tintMod, 190 - tintMod, 255 - tintMod),
                           CCTintTo::create(tintDuration, 40 - tintMod, 255 - tintMod, 133 - tintMod),
                           CCTintTo::create(tintDuration, 120 - tintMod, 255 - tintMod, 40 - tintMod),
                           CCTintTo::create(tintDuration, 255 - tintMod, 208 - tintMod, 40 - tintMod),
                           CCTintTo::create(tintDuration, 255 - tintMod, 47 - tintMod, 40 - tintMod),
                           CCTintTo::create(tintDuration, 255 - tintMod, 40 - tintMod, 241 - tintMod),
                           CCTintTo::create(tintDuration, 65 - tintMod, 40 - tintMod, 255 - tintMod),
                           CCTintTo::create(tintDuration, 40 - tintMod, 126 - tintMod, 255 - tintMod), nullptr));

    constexpr float buttonScale = 1.1f;

    auto clippingNode = Build<CCClippingNode>::create().zOrder(-5).parent(m_mainLayer).collect();

    auto mask = CCDrawNode::create();
    mask->drawPolygon(rectangle, 4, ccc4FFromccc3B({0, 0, 0}), 0, ccc4FFromccc3B({0, 0, 0}));
    clippingNode->setStencil(mask);

    m_background = Build(cue::RepeatingBackground::create("game_bg_02_001.png", 0.6f, cue::RepeatMode::X, m_size))
                       .id("background")
                       .color({40, 125, 255})
                       .parent(clippingNode);

    // Build(util::ui::makeRepeatingBackground(
    //     "game_bg_02_001.png",
    //     {40, 125, 255},
    //     1.f, 0.6f, util::ui::RepeatMode::X, m_size)
    // )
    //     .id("background")
    //     .parent(clippingNode)
    //     .store(this->background)
    //     .collect();

    m_background->runAction(bgSequence);

    m_ground = Build(cue::RepeatingBackground::create("groundSquare_02_001.png", 0.6f, cue::RepeatMode::X, m_size))
                   .with([&](auto bg) { bg->setSpeed(10.f); })
                   .id("ground")
                   .posY(-10.f)
                   .color({40 - tintMod, 125 - tintMod, 255 - tintMod})
                   .zOrder(6)
                   .parent(clippingNode);

    m_ground->runAction(groundSequence);

    ccBlendFunc blending = {GL_ONE, GL_ONE};

    Build<CCSprite>::createSpriteName("floorLine_001.png")
        .id("floor-line")
        .pos(this->center().x, 66.f)
        .zOrder(5)
        .scale(0.9f)
        .blendFunc(blending)
        .parent(m_mainLayer);

    float promoCardFinalScale = 0.85f;
    if (winSize.width < 520.f) {
        promoCardFinalScale = (promoCardFinalScale - (520.f - winSize.width) / 1000.f);
    }

    auto *promoCard = Build<CCSprite>::create("kofi-promo-card.png"_spr)
                          .anchorPoint(1.f, 1.f)
                          .scale(0.5f)
                          .pos(this->fromTopRight(25.f, 30.f))
                          .parent(m_mainLayer)
                          .with([&](CCSprite *card) {
                              auto scaleUpAction =
                                  CCEaseExponentialOut::create(CCScaleTo::create(1.25f, promoCardFinalScale));
                              card->runAction(scaleUpAction);
                          })
                          .collect();

    // create kofi btn

    Build<ButtonSprite>::create("Open Ko-fi", "goldFont.fnt", "GJ_button_03.png", 1.f)
        .intoMenuItem(+[] { geode::utils::web::openLinkInBrowser("https://ko-fi.com/globed"); })
        .intoNewParent(CCMenu::create())
        .pos(this->fromBottom(5.f))
        .anchorPoint(0.f, 0.f)
        .with([&](CCMenu *menu) {
            auto buttonSequence = CCRepeatForever::create(
                CCSequence::create(CCEaseSineInOut::create(CCScaleBy::create(0.85f, buttonScale)),
                                   CCEaseSineInOut::create(CCScaleBy::create(0.85f, 1.f / buttonScale)), nullptr));

            menu->runAction(buttonSequence);
        })
        .parent(m_mainLayer);

    auto *gm = GameManager::sharedState();

    cue::PlayerIcon *player;
    Build<cue::PlayerIcon>::create(globed::getPlayerIcons())
        .anchorPoint(0.5f, 0.5f)
        .pos(-65.f, 81.5f)
        .parent(clippingNode)
        .with([&](cue::PlayerIcon *player) {
            auto playerSequence = CCSequence::create(
                CCDelayTime::create(0.25f),
                CCCallFunc::create(this, callfunc_selector(SupportPopup::kofiEnableParticlesCallback)),
                CCEaseExponentialOut::create(CCMoveBy::create(1.25f, {155.f, 0.f})), nullptr);

            player->runAction(playerSequence);
        })
        .store(player)
        .intoNewChild(NameLabel::create(gm->m_playerName, "chatFont.fnt"))
        .with([&](NameLabel *label) {
            label->addBadge(createBadge("role-supporter.png"));
            label->updateColor(Color3{154, 88, 255});
        })
        .pos(15.f, 45.f);

    auto emitter = Build<CCParticleSystemQuad>::create()
                       .pos(0.f, 0.f)
                       .anchorPoint(0.f, 0.f)
                       .scale(0.9f)
                       .zOrder(-1)
                       .store(m_particles)
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

void SupportPopup::update(float dt)
{
    m_particles->setPosition(0.f, 0.f);
}

void SupportPopup::kofiEnableParticlesCallback()
{
    this->scheduleOnce(schedule_selector(SupportPopup::kofiEnableParticlesCallback2), 0.1f);
}

void SupportPopup::kofiEnableParticlesCallback2(float dt)
{
#ifdef GEODE_IS_WINDOWS
    m_particles->setOpacity(255);
#else
    m_particles->setScale(0.9f);
#endif
}

SupportPopup *SupportPopup::create()
{
    auto bg = CCSprite::create("kofi-promo-border.png"_spr);
    float scaleMult = std::min(1.5f, (CCDirector::get()->getWinSize().width - 35.f) / bg->getContentWidth());
    bg->setScale(scaleMult);
    POPUP_SIZE = bg->getScaledContentSize();

    return BasePopup::create(bg);
}

} // namespace globed