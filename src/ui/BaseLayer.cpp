#include "BaseLayer.hpp"
#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

bool BaseLayer::init(bool background) {
    if (!CCLayer::init()) return false;

    this->setKeyboardEnabled(true);
    this->setKeypadEnabled(true);

    auto winSize = CCDirector::get()->getWinSize();

    m_backButton = Build<CCSprite>::createSpriteName("GJ_arrow_01_001.png")
        .intoMenuItem([this](auto) {
            this->keyBackClicked();
        })
        .id("back-button");

    m_backMenu = Build<CCMenu>::create()
        .id("back-menu")
        .child(m_backButton)
        .contentSize(m_backButton->getScaledContentSize())
        .anchorPoint(0.f, 1.f)
        .pos(25.f, winSize.height - 25.f)
        .parent(this)
        .collect();

    if (background) {
        this->initBackground();
    }

    return true;
}

void BaseLayer::initBackground(cocos2d::ccColor3B color) {
    if (m_background) {
        m_background->removeFromParent();
        m_background = nullptr;
    }

    auto winSize = CCDirector::get()->getWinSize();
    auto bg = CCSprite::create("GJ_gradientBG.png");
    auto bgSize = bg->getTextureRect().size;

    Build<CCSprite>(bg)
        .id("background")
        .anchorPoint(0.f, 0.f)
        .scaleX((winSize.width + 10.f) / bgSize.width)
        .scaleY((winSize.height + 10.f) / bgSize.height)
        .pos(-5.f, -5.f)
        .color(color)
        .zOrder(-1)
        .parent(this)
        .store(m_background);
}

void BaseLayer::keyBackClicked() {
    CCDirector::get()->popSceneWithTransition(0.5f, PopTransition::kPopTransitionFade);
}

void BaseLayer::switchTo() {
    auto scene = CCScene::create();
    scene->addChild(this);

    CCDirector::get()->pushScene(
        CCTransitionFade::create(0.5f, scene)
    );
}

}