#include "ui.hpp"

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace util::ui {
    void addBackground(cocos2d::CCNode* layer) {
        auto windowSize = CCDirector::get()->getWinSize();

        auto bg = CCSprite::create("GJ_gradientBG.png");
        auto bgSize = bg->getTextureRect().size;

        Build<CCSprite>(bg)
            .anchorPoint({0.f, 0.f})
            .scaleX((windowSize.width + 10.f) / bgSize.width)
            .scaleY((windowSize.height + 10.f) / bgSize.height)
            .pos({-5.f, -5.f})
            .color({0, 102, 255})
            .zOrder(-1)
            .parent(layer);
    }

    void addBackButton(CCMenu* menu, std::function<void()> callback) {
        auto windowSize = CCDirector::get()->getWinSize();
        Build<CCSprite>::createSpriteName("GJ_arrow_01_001.png")
            .intoMenuItem([callback](CCObject*) {
                callback();
            })
            .pos(-windowSize.width / 2 + 25.0f, windowSize.height / 2 - 25.0f)
            .parent(menu);
    }

    void navigateBack() {
        auto director = CCDirector::get();
        director->popSceneWithTransition(0.5f, PopTransition::kPopTransitionFade);
    }
}