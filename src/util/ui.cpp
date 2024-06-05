#include "ui.hpp"

#include <hooks/game_manager.hpp>
#include <managers/settings.hpp>
#include <managers/role.hpp>
#include <util/cocos.hpp>
#include <util/format.hpp>
#include <util/misc.hpp>
#include <util/time.hpp>
#include <util/math.hpp>

using namespace geode::prelude;

namespace util::ui {
    void switchToScene(CCScene* scene) {
        CCDirector::get()->pushScene(CCTransitionFade::create(.5f, scene));
    }

    void switchToScene(CCLayer* layer) {
        switchToScene(Build<CCScene>::create().child(layer).collect());
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

    static void scrollToBottom(CCContentLayer* layer) {
        layer->setPositionY(0.f);
    }

    static void scrollToTop(CCContentLayer* layer) {
        layer->setPositionY(
            layer->getParent()->getScaledContentSize().height - layer->getScaledContentSize().height
        );
    }

    void scrollToBottom(BoomListView* listView) {
        scrollToBottom(listView->m_tableView->m_contentLayer);
    }

    void scrollToBottom(ScrollLayer* listView) {
        scrollToBottom(listView->m_contentLayer);
    }

    void scrollToTop(BoomListView* listView) {
        scrollToTop(listView->m_tableView->m_contentLayer);
    }

    void scrollToTop(ScrollLayer* listView) {
        scrollToTop(listView->m_contentLayer);
    }

    void tryLoadDeathEffect(int id) {
        if (id <= 1) return;

        auto textureCache = CCTextureCache::sharedTextureCache();
        auto sfCache  = CCSpriteFrameCache::sharedSpriteFrameCache();

        auto pngKey = fmt::format("PlayerExplosion_{:02}.png", id - 1);
        auto plistKey = fmt::format("PlayerExplosion_{:02}.plist", id - 1);

        if (textureCache->textureForKey(pngKey.c_str()) == nullptr) {
            textureCache->addImage(pngKey.c_str(), false);
            sfCache->addSpriteFramesWithFile(plistKey.c_str());
        }
    }

    PopupLayout getPopupLayout(CCSize popupSize) {
        PopupLayout layout;

        layout.winSize = CCDirector::get()->getWinSize();
        layout.popupSize = popupSize;
        layout.center = CCSize{layout.winSize.width / 2, layout.winSize.height / 2};

        layout.left = layout.center.width - popupSize.width / 2;
        layout.bottom = layout.center.height - popupSize.height / 2;
        layout.right = layout.center.width + popupSize.width / 2;
        layout.top = layout.center.height + popupSize.height / 2;

        layout.centerLeft = CCSize{layout.left, layout.center.height};
        layout.centerRight = CCSize{layout.right, layout.center.height};
        layout.centerTop = CCSize{layout.center.width, layout.top};
        layout.centerBottom = CCSize{layout.center.width, layout.bottom};

        layout.bottomLeft = CCSize{layout.left, layout.bottom};
        layout.topLeft = CCSize{layout.left, layout.top};
        layout.bottomRight = CCSize{layout.right, layout.bottom};
        layout.topRight = CCSize{layout.right, layout.top};

        return layout;
    }

    CCNode* findChildByMenuSelectorRecursive(CCNode* node, uintptr_t function) {
        if (auto button = typeinfo_cast<CCMenuItem*>(node)) {
            uintptr_t btnAddr = *(uintptr_t*)&button->m_pfnSelector;
            if (function == btnAddr) {
                return node;
            }
        }

        if (!node->getChildren() || node->getChildrenCount() == 0) return nullptr;

        for (CCNode* child : CCArrayExt<CCNode*>(node->getChildren())) {
            auto result = findChildByMenuSelectorRecursive(child, function);
            if (result) {
                return result;
            }
        }

        return nullptr;
    }

    CCSprite* createBadge(const std::string& sprite) {
        // have multiple fallback sprites in case it's invalid
        CCSprite* spr1 = nullptr;

        if (!sprite.empty()) spr1 = CCSprite::createWithSpriteFrameName(util::cocos::spr(sprite).c_str());
        if (!spr1 && !sprite.empty()) spr1 = CCSprite::createWithSpriteFrameName(sprite.c_str());
        if (!spr1) spr1 = CCSprite::createWithSpriteFrameName(util::cocos::spr("button-secret.png").c_str());

        return spr1;
    }

    static ComputedRole compute(const SpecialUserData& data) {
        return RoleManager::get().compute(data.roles.value());
    }

    CCSprite* createBadgeIfSpecial(const SpecialUserData& data) {
        if (!data.roles) return nullptr;

        return createBadge(compute(data).badgeIcon);
    }

    ccColor3B getNameColor(const SpecialUserData& data) {
        if (!data.roles) return ccc3(255, 255, 255);

        auto computed = compute(data);
        if (computed.nameColor) return computed.nameColor->getAnyColor();

        return ccc3(255, 255, 255);
    }

    RichColor getNameRichColor(const SpecialUserData& data) {
        if (!data.roles) return RichColor(ccc3(255, 255, 255));

        auto computed = compute(data);
        if (computed.nameColor) return computed.nameColor.value();

        return RichColor(ccc3(255, 255, 255));
    }

    void animateLabelColorTint(cocos2d::CCLabelBMFont* label, const RichColor& color) {
        constexpr int tag = 34925671;

        label->stopActionByTag(tag);

        if (!color.isMultiple()) {
            label->setColor(color.getColor());
            return;
        }

        const auto& colors = color.getColors();
        if (colors.empty()) {
            return;
        }

        // set the last color
        label->setColor(colors.at(colors.size() - 1));

        // create an action to tint between the rest of the colors
        CCArray* actions = CCArray::create();

        for (const auto& color : colors) {
            actions->addObject(CCTintTo::create(0.8f, color.r, color.g, color.b));
        }

        CCRepeat* action = CCRepeat::create(CCSequence::create(actions), 99999999);
        action->setTag(tag);

        label->runAction(action);
    }
}