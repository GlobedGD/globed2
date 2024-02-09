#include "ui.hpp"

#include <util/format.hpp>
#include <util/misc.hpp>
#include <util/time.hpp>
#include <util/math.hpp>

using namespace geode::prelude;

namespace util::ui {
    void switchToScene(CCLayer* layer) {
        // massive geode L
        // uses replaceScene instead of pushScene
        auto scene = CCScene::create();
        scene->addChild(layer);
        CCDirector::get()->pushScene(CCTransitionFade::create(.5f, scene));
    }

    void prepareLayer(CCLayer* layer) {
        util::ui::addBackground(layer);

        auto menu = CCMenu::create();
        layer->addChild(menu);

        util::ui::addBackButton(menu, util::ui::navigateBack);

        layer->setKeyboardEnabled(true);
        layer->setKeypadEnabled(true);
    }

    void addBackground(CCNode* layer) {
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

    void rescaleToMatch(cocos2d::CCNode* node, cocos2d::CCNode* target, bool stretch) {
        auto targetSize = target->getScaledContentSize();
        rescaleToMatch(node, targetSize, stretch);
    }

    void rescaleToMatch(cocos2d::CCNode *node, cocos2d::CCSize targetSize, bool stretch) {
        auto nodeSize = node->getContentSize();

        if (!stretch) {
            float scale = targetSize.width / nodeSize.width;
            node->setScale(scale);
        } else {
            node->setScaleX(targetSize.width / nodeSize.width);
            node->setScaleY(targetSize.height / nodeSize.height);
        }
    }

    float getScrollPos(BoomListView* listView) {
        auto* cl = listView->m_tableView->m_contentLayer;
        if (cl->getPositionY() > 0.f) return 99999.f;
        return cl->getScaledContentSize().height + cl->getPositionY();
    }

    void setScrollPos(BoomListView* listView, float pos) {
        if (util::math::equal(pos, 99999.f)) return;

        auto* cl = listView->m_tableView->m_contentLayer;
        float actualPos = pos - cl->getScaledContentSize().height;

        cl->setPositionY(std::min(actualPos, 0.f));
    }

// [from; to]
#define LOAD_ICON_RANGE(type, from, to) \
    for (int i = from; i <= to; i++) { \
        gm->loadIcon(i, (int)IconType::type, -1); \
    }

    void preloadAssets() {
        auto start = util::time::now();
        log::debug("Preloading death effects..");
        for (int i = 1; i < 21; i++) {
            tryLoadDeathEffect(i);
        }

        log::debug("Took {}.", util::format::formatDuration(util::time::now() - start));
        auto start2 = util::time::now();
        log::debug("Preloading icons..");

        auto* gm = GameManager::get();
        LOAD_ICON_RANGE(Cube, 0, 484);
        LOAD_ICON_RANGE(Ship, 0, 169);
        LOAD_ICON_RANGE(Ball, 0, 118);
        LOAD_ICON_RANGE(Ufo, 0, 149);
        LOAD_ICON_RANGE(Wave, 0, 96);
        LOAD_ICON_RANGE(Robot, 0, 68);
        LOAD_ICON_RANGE(Spider, 0, 69);
        LOAD_ICON_RANGE(Swing, 0, 43);
        LOAD_ICON_RANGE(Jetpack, 0, 5);

        auto end = util::time::now();
        log::debug("Took {}.", util::format::formatDuration(end - start2));
        log::debug("Preloading finished in {}.", util::format::formatDuration(end - start));
        // robot and spider idk if necessary
    }

    void tryLoadDeathEffect(int id) {
        if (id == 1) return;

        auto textureCache = CCTextureCache::sharedTextureCache();
        auto sfCache  = CCSpriteFrameCache::sharedSpriteFrameCache();

        auto pngKey = fmt::format("PlayerExplosion_{:02}.png", id - 1);
        auto plistKey = fmt::format("PlayerExplosion_{:02}.plist", id - 1);

        if (textureCache->textureForKey(pngKey.c_str()) == nullptr) {
            textureCache->addImage(pngKey.c_str(), false);
            sfCache->addSpriteFramesWithFile(plistKey.c_str());
        }
    }
}