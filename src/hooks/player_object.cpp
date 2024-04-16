#include "player_object.hpp"

#include <hooks/gjbasegamelayer.hpp>
#include <util/ui.hpp>
#include <util/debug.hpp>

using namespace geode::prelude;

void ComplexPlayerObject::setRemotePlayer(ComplexVisualPlayer* rp) {
    this->setUserObject(rp);

    // also set remote state
    this->setTag(COMPLEX_PLAYER_OBJECT_TAG);
}

bool ComplexPlayerObject::vanilla() {
    return this->getTag() != COMPLEX_PLAYER_OBJECT_TAG;
}

void ComplexPlayerObject::playDeathEffect() {
    if (vanilla()) {
        PlayerObject::playDeathEffect();
        return;
    }

    auto* rp = static_cast<ComplexVisualPlayer*>(this->getUserObject());
    int deathEffect = rp->storedIcons.deathEffect;

    auto* gm = GameManager::get();

    // we need to do this because the orig func reads the death effect ID from GameManager
    int oldEffect = gm->getPlayerDeathEffect();
    gm->setPlayerDeathEffect(deathEffect);

    // also make sure the death effect is properly loaded
    util::ui::tryLoadDeathEffect(deathEffect);

    PlayerObject::playDeathEffect();
    gm->setPlayerDeathEffect(oldEffect);
}

void ComplexPlayerObject::incrementJumps() {
    if (vanilla()) PlayerObject::incrementJumps();
}


void HookedPlayerObject::playSpiderDashEffect(cocos2d::CCPoint from, cocos2d::CCPoint to) {
    // if we are in the editor, do nothing
    if (GJBaseGameLayer::get() == nullptr) {
        PlayerObject::playSpiderDashEffect(from, to);
        return;
    }

    auto* gpl = GlobedGJBGL::get();
    if (this == gpl->m_player1) {
        gpl->m_fields->spiderTp1 = SpiderTeleportData { .from = from, .to = to };
    } else if (this == gpl->m_player2) {
        gpl->m_fields->spiderTp2 = SpiderTeleportData { .from = from, .to = to };
    }

    PlayerObject::playSpiderDashEffect(from, to);
}

void HookedPlayerObject::incrementJumps() {
    if (GJBaseGameLayer::get() == nullptr || m_isOnSlope) {
        PlayerObject::incrementJumps();
        return;
    }

    auto* gpl = GlobedGJBGL::get();

    if (this == gpl->m_player1) {
        gpl->m_fields->didJustJumpp1 = true;
    } else if (this == gpl->m_player2) {
        gpl->m_fields->didJustJumpp2 = true;
    }

    PlayerObject::incrementJumps();
}

void HookedPlayerObject::update(float dt) {
    auto* gjbgl = GJBaseGameLayer::get();
    if (!gjbgl) {
        PlayerObject::update(dt);
        return;
    }

    auto* gpl = GlobedGJBGL::get();
    if (gpl->m_fields->forcedPlatformer) {
        this->togglePlatformerMode(true);

#ifdef GEODE_IS_ANDROID
        if (!m_fields->forcedPlatFlag) {
            if (gpl->m_uiLayer) {
                m_fields->forcedPlatFlag = true;
                gpl->m_uiLayer->togglePlatformerMode(true);
            }
        }
#endif
    }

    PlayerObject::update(dt);
}

// decompiled playerobject init for reference:

/*

bool HookedPlayerObject::init(int param1, int param2, GJBaseGameLayer* gameLayer, cocos2d::CCLayer* parentLayer, bool someBool) {
    param1 = std::clamp(param1, 1, 0x1e4);
    param2 = std::clamp(param2, 1, 0xa9);

    auto* gm = GameManager::sharedState();
    m_iconRequestID = gm->getIconRequestID();

    gm->loadIcon(param1, 0, m_iconRequestID);
    gm->loadIcon(param1, 1, m_iconRequestID);

    auto local6c = fmt::format("player_{:02}_001.png", param1);
    auto local68 = fmt::format("player_{:02}_2_001.png", param1);

    // inlined GameObject::init
    if (!CCSprite::initWithSpriteFrameName(local6c.c_str())) return false;

    GameObject::commonSetup();
    m_bUnkBool2 = true;
    m_bUnkBool2 = false; // lmao
    m_bDontDraw = true; // ??

    m_unk49c = CCNode::create();
    // vfunc 38 on armv7 - setContentSize
    m_unk49c->setContentSize(CCSize{0.f, 0.f});
    // vfunc 55 on armv7 (56 on win) - addChild
    this->addChild(m_unk49c);
    m_unk930 = "run";
    m_unk6a2 = gm->getGameVariable("0096");
    m_unk6a3 = gm->getGameVariable("0100");
    m_unk948 = gm->getGameVariable("0123");

    auto* gsm = GameStatsManager::sharedState();
    m_unk979 = gsm->isItemEnabled((UnlockType)0xc, 0x12);
    m_unk97a = gsm->isItemEnabled((UnlockType)0xc, 0x13);
    m_unk97b = gsm->isItemEnabled((UnlockType)0xc, 0x14);
    m_hasGhostTrail = 0;

    // unknown member
    // windows mbo, androidv7 is 0x520
    *(int*)((char*)this + 0x530) = param1;
    m_playerSpeed = 0.9f;

    auto iVar9 = rand();
    m_unk91c = (rand() / 32767.f) * 10.f + 5.f;
    m_gameLayer = gameLayer;
    m_parentLayer = parentLayer;
    m_unk65c = someBool;
    m_touchingRings = CCArray::create();
    m_touchingRings->retain();

    // vfunc 122 on windows
    this->setTextureRect({0.f, 0.f, 0.f, 0.f});

    m_iconSprite = CCSprite::createWithSpriteFrameName(local6c.c_str());
    m_unk49c->addChild(m_iconSprite, 1);

    m_iconSpriteSecondary = CCSprite::createWithSpriteFrameName(local68.c_str());
    m_iconSprite->addChild(m_iconSpriteSecondary, -1);

    // vfunc 23 on windows/androidv7
    m_iconSpriteSecondary->setPosition(m_iconSprite->convertToNodeSpace({0.f, 0.f}));

    m_iconSpriteWhitener = CCSprite::createWithSpriteFrameName(local68.c_str());
    // vfunc 56 on androidv7
    m_iconSprite->addChild(m_iconSpriteWhitener, 2);

    // vfunc 23 on windows/androidv7
    m_iconSpriteWhitener->setPosition(m_iconSprite->convertToNodeSpace({0.f, 0.f}));

    auto auStack64 = fmt::format("player_{:02}_extra_001.png", param1);
    this->updatePlayerSpriteExtra(auStack64);

    auto local60 = fmt::format("ship_{:02}_001.png", param2);
    auto local5c = fmt::format("ship_{:02}_2_001.png", param2);

    m_vehicleSprite = CCSprite::createWithSpriteFrameName(local60.c_str());
    m_unk49c->addChild(m_vehicleSprite, 2);

    // vfunc 40 on windows, 41 on androidv7
    m_vehicleSprite->setVisible(false);
    m_vehicleSpriteSecondary = CCSprite::createWithSpriteFrameName(local5c.c_str());
    m_vehicleSprite->addChild(m_vehicleSpriteSecondary, -1);
    m_vehicleSpriteSecondary->setPosition(m_vehicleSprite->convertToNodeSpace({0.f, 0.f}));

    m_unk604 = CCSprite::createWithSpriteFrameName(local5c.c_str());
    m_vehicleSprite->addChild(m_unk604, -2);
    m_unk604->setPosition(m_vehicleSprite->convertToNodeSpace({0.f, 0.f}));

    m_unk604->setVisible(false);

    m_vehicleSpriteWhitener = CCSprite::createWithSpriteFrameName(local5c.c_str());
    m_vehicleSprite->addChild(m_vehicleSpriteWhitener, 1);
    m_vehicleSpriteWhitener->setPosition(m_vehicleSprite->convertToNodeSpace({0.f, 0.f}));

    auto local44 = fmt::format("player_{:02}_extra_001.png", param2);
    this->updateShipSpriteExtra(local44);

    this->createRobot(gm->getPlayerRobot());

    m_robotFire = PlayerFireBoostSprite::create();
    m_unk49c->addChild(m_robotFire, -1);
    m_robotFire->setVisible(false);

    this->createSpider(gm->getPlayerSpider());

    // add 3 swing fires
    for (size_t i = 0; i < 3; i++) {
        auto fire = PlayerFireBoostSprite::create();
        m_unk49c->addChild(fire, -1);

        float fVar19 = i == 0 ? 0.8f : 0.6f;
        fire->m_size = fVar19 * 1.2f;
        fire->setVisible(false);

        if (i == 0) {
            m_swingFireMiddle = fire;
        } else if (i == 1) {
            m_swingFireBottom = fire;
        } else {
            m_swingFireTop = fire;
        }
    }

    // vfunc 42 on windows, 43 on androidv7
    m_swingFireMiddle->setRotation(90.f);
    m_swingFireMiddle->setPosition({-14.f, 0.f});

    m_swingFireBottom->setRotation(45.f);
    m_swingFireBottom->setPosition({-9.5f, -10.f});

    m_swingFireTop->setRotation(135.f);
    m_swingFireTop->setPosition({-9.5f, 10.f});

#ifdef GEODE_IS_ANDROID
    this->m_yVelocity = 0.0;
#else
    // TODO is this member misaligned??
    *(double*)((char*)this + 0x790) = 0.0;
#endif
    m_isDead = false;
    m_isOnGround = false;
    m_unk7f8 = false;

    // TODO idk bool but same offset on both platforms
    *(bool*)((char*)this + 0x80c) = false;

    m_vehicleSize = 1.0f;
    m_unk838 = 30.f;

    this->updateTimeMod(0.9f, false);

    m_unk6c0 = 0;
    m_unk4e4 = CCNode::create();
    m_unk4e4->retain();
    m_unk4e8 = CCDictionary::create();
    m_unk4e8->retain();
    m_unk4ec = CCDictionary::create();
    m_unk4ec->retain();
    m_unk4f0 = CCDictionary::create();
    m_unk4f0->retain();
    m_unk4f4 = CCDictionary::create();
    m_unk4f4->retain();
    m_particleSystems = CCArray::create();
    m_particleSystems->retain();
    m_actionManager = GJActionManager::create();
    m_actionManager->retain();

    m_unk6dc = CCParticleSystemQuad::create("dragEffect.plist", false);
    // vfunc 189 on windows
    m_unk6dc->setPositionType(tCCPositionType::kCCPositionTypeRelative);
    m_unk6dc->stopSystem();
    m_particleSystems->addObject(m_unk6dc);
    m_unk65d = false;

    m_unk6f4 = CCParticleSystemQuad::create("dashEffect.plist", false);
    m_unk6f4->setPositionType(tCCPositionType::kCCPositionTypeRelative);
    m_unk6f4->stopSystem();
    m_particleSystems->addObject(m_unk6f4);

    m_ufoClickParticles = CCParticleSystemQuad::create("burstEffect.plist", false);
    m_ufoClickParticles->setPositionType(tCCPositionType::kCCPositionTypeRelative);
    m_ufoClickParticles->stopSystem();
    m_particleSystems->addObject(m_ufoClickParticles);

    m_robotBurstParticles = CCParticleSystemQuad::create("burstEffect.plist", false);
    m_robotBurstParticles->setPositionType(tCCPositionType::kCCPositionTypeRelative);
    m_robotBurstParticles->stopSystem();
    m_particleSystems->addObject(m_robotBurstParticles);

    m_trailingParticles = CCParticleSystemQuad::create("dragEffect.plist", false);
    m_trailingParticles->setPositionType(tCCPositionType::kCCPositionTypeRelative);
    m_trailingParticles->stopSystem();
    m_particleSystems->addObject(m_trailingParticles);
    // vfunc 118 on windows
    m_unk648 = m_trailingParticles->CCParticleSystem::getLife();
    // vfunc 117 on windows
    m_trailingParticles->CCParticleSystem::setPosVar({0.0f, 2.0f});

    m_shipClickParticles = CCParticleSystemQuad::create("dragEffect.plist", false);
    m_shipClickParticles->setPositionType(tCCPositionType::kCCPositionTypeRelative);
    m_shipClickParticles->stopSystem();
    m_particleSystems->addObject(m_shipClickParticles);
    // 0x6d8 is m_trailingParticles
    // vfunc 129 on windows
    m_trailingParticles->CCParticleSystem::setSpeed(m_trailingParticles->CCParticleSystem::getSpeed() * 0.2f);
    m_trailingParticles->CCParticleSystem::setSpeedVar(m_trailingParticles->CCParticleSystem::getSpeedVar() * 0.2f);

    // vfunc 117 on windows
    m_shipClickParticles->CCParticleSystem::setPosVar({0.0f, 2.0f});
    m_shipClickParticles->CCParticleSystem::setSpeed(m_shipClickParticles->CCParticleSystem::getSpeed() * 2.0f);
    m_shipClickParticles->CCParticleSystem::setSpeedVar(m_shipClickParticles->CCParticleSystem::getSpeedVar() * 2.0f);
    // vfuncs on windows 125, 124
    m_shipClickParticles->CCParticleSystem::setAngleVar(m_shipClickParticles->CCParticleSystem::getAngleVar() * 2.0f);
    m_shipClickParticles->CCParticleSystem::setStartSize(m_shipClickParticles->CCParticleSystem::getStartSize() * 1.5f);
    m_shipClickParticles->CCParticleSystem::setStartSizeVar(m_shipClickParticles->CCParticleSystem::getStartSizeVar() * 1.5f);
    m_unk65e = false;

    // vfunc 167 on windows
    // TODO: color order might be backwards
    m_trailingParticles->CCParticleSystem::setStartColor(ccc4f(1.0f, 0.39215f, 0.f, 1.f));
    m_trailingParticles->CCParticleSystem::setEndColor(ccc4f(1.0f, 0.0f, 0.f, 1.f));

    m_shipClickParticles->CCParticleSystem::setStartColor(ccc4f(1.0f, 0.745f, 0.f, 1.f));
    m_shipClickParticles->CCParticleSystem::setEndColor(ccc4f(1.0f, 0.0f, 0.f, 1.f));

    for (size_t i = 0; i < 2; i++) {
        auto effect = CCParticleSystemQuad::create("swingBurstEffect.plist", false);
        effect->setPositionType(tCCPositionType::kCCPositionTypeGrouped);
        m_particleSystems->addObject(effect);
        effect->stopSystem();

        // TODO keep colors in mind
        effect->CCParticleSystem::setStartColor(ccc4f(1.0f, 0.39215f, 0.f, 1.f));
        effect->CCParticleSystem::setEndColor(ccc4f(1.0f, 0.0f, 0.f, 1.f));

        if (i == 0) {
            m_swingBurstParticles1 = effect;
        } else {
            m_swingBurstParticles2 = effect;
        }
    }

    m_unk6e8 = CCParticleSystemQuad::create("shipDragEffect.plist", false);
    m_unk6e8->setPositionType(tCCPositionType::kCCPositionTypeGrouped);
    m_particleSystems->addObject(m_unk6e8);
    m_unk6e8->stopSystem();

    m_unk704 = CCParticleSystemQuad::create("landEffect.plist", false);
    m_unk704->setPositionType(tCCPositionType::kCCPositionTypeGrouped);
    m_particleSystems->addObject(m_unk704);
    m_unk704->stopSystem();

    // vfunc 122 on windows
    m_unk70c = m_unk704->CCParticleSystem::getAngle();
    // vfunc 127 on windows
    m_unk710 = m_unk704->CCParticleSystem::getGravity().y;

    m_unk708 = CCParticleSystemQuad::create("landEffect.plist", false);
    m_unk708->setPositionType(tCCPositionType::kCCPositionTypeGrouped);
    m_particleSystems->addObject(m_unk708);
    m_unk708->stopSystem();
    this->setupStreak();

    auto local58 = fmt::format("player_{:02}_glow_001.png", param1);
    m_unk61c = CCSprite::create();

    // vfunc 122 on windows
    m_unk61c->setTextureRect({0.f, 0.f, 0.f, 0.f});
    m_unk61c->setDontDraw(true);

    // vfunc 55 on windows
    m_unk49c->addChild(m_unk61c, -1);

    m_unk56c = CCSprite::createWithSpriteFrameName("playerDash2_001.png");
    m_unk56c->setBlendFunc(_ccBlendFunc {0x302, 1});

    m_unk61c->addChild(m_unk56c);
    // vfunc 23 on windows
    m_unk56c->setPosition(this->convertToNodeSpace({0.f, 0.f}));
    m_unk56c->setVisible(false);

    auto dashOutline = CCSprite::createWithSpriteFrameName("playerDash2_outline_001.png");
    m_unk56c->addChild(dashOutline, 1);

    dashOutline->setPosition(m_unk56c->convertToNodeSpace({0.f, 0.f}));
    dashOutline->setOpacity(0x96);

    m_iconGlow = CCSprite::createWithSpriteFrameName(local58.c_str());
    // vfunc 40 on windows
    m_iconGlow->setVisible(false);
    m_unk61c->addChild(m_iconGlow, 2);

    auto local4c = fmt::format("ship_{:02}_glow_001.png", param2);
    m_vehicleGlow = CCSprite::createWithSpriteFrameName(local4c.c_str());
    m_unk61c->addChild(m_vehicleGlow, -3);
    m_vehicleGlow->setVisible(false);

    m_unk288 = 30.f;
    m_unk28c = 30.f;
    m_unk838 = 30.f;

    m_gamevar0060 = gm->getGameVariable("0060");
    m_gamevar0061 = gm->getGameVariable("0061");
    m_gamevar0062 = gm->getGameVariable("0062");

    // vector thingy idk
    m_unk880.resize(200);

    m_unk51d = false;

    return true;
}


*/