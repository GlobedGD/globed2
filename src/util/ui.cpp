#include "ui.hpp"

#include <hooks/game_manager.hpp>
#include <data/types/admin.hpp>
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

    void replaceScene(CCScene* layer) {
        CCDirector::get()->replaceScene(CCTransitionFade::create(.5f, layer));
    }

    void replaceScene(CCLayer* layer) {
        replaceScene(Build<CCScene>::create().child(layer).collect());
    }

    void prepareLayer(CCLayer* layer, cocos2d::ccColor3B color) {
        util::ui::addBackground(layer, color);

        auto menu = CCMenu::create();
        layer->addChild(menu);

        util::ui::addBackButton(menu, util::ui::navigateBack);

        layer->setKeyboardEnabled(true);
        layer->setKeypadEnabled(true);
    }

    void addBackground(CCNode* layer, cocos2d::ccColor3B color) {
        auto windowSize = CCDirector::get()->getWinSize();

        auto bg = CCSprite::create("GJ_gradientBG.png");
        auto bgSize = bg->getTextureRect().size;

        Build<CCSprite>(bg)
            .anchorPoint({0.f, 0.f})
            .scaleX((windowSize.width + 10.f) / bgSize.width)
            .scaleY((windowSize.height + 10.f) / bgSize.height)
            .pos({-5.f, -5.f})
            .color(color)
            .zOrder(-1)
            .id("background")
            .parent(layer);
    }

    void addBackButton(CCMenu* menu, std::function<void()> callback) {
        auto windowSize = CCDirector::get()->getWinSize();
        Build<CCSprite>::createSpriteName("GJ_arrow_01_001.png")
            .intoMenuItem([callback](CCObject*) {
                callback();
            })
            .id("back-button")
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

    static PopupLayout popupLayoutWith(const CCSize& popupSize, bool useWinSize) {
        PopupLayout layout;

        layout.winSize = CCDirector::get()->getWinSize();
        layout.popupSize = popupSize;

        if (useWinSize) {
            layout.center = CCSize{layout.winSize.width / 2, layout.winSize.height / 2};
        } else {
            layout.center = CCSize{layout.popupSize.width / 2, layout.popupSize.height / 2};
        }

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

    PopupLayout getPopupLayout(const CCSize& popupSize) {
        return popupLayoutWith(popupSize, true);
    }

    PopupLayout getPopupLayoutAnchored(const CCSize& popupSize) {
        return popupLayoutWith(popupSize, false);
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

    static ComputedRole compute(const UserEntry& data) {
        return RoleManager::get().compute(data.userRoles);
    }

    CCSprite* createBadgeIfSpecial(const SpecialUserData& data) {
        if (!data.roles) return nullptr;

        return createBadge(compute(data).badgeIcon);
    }

    CCSprite* createBadgeIfSpecial(const UserEntry& data) {
        if (data.userRoles.empty()) return nullptr;

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

    void makeListGray(GJListLayer* listLayer) {
        auto top = static_cast<CCSprite*>(geode::cocos::getChildBySpriteFrameName(listLayer, "GJ_table_top_001.png"));
        auto bottom = static_cast<CCSprite*>(geode::cocos::getChildBySpriteFrameName(listLayer, "GJ_table_bottom_001.png"));

        CCSprite *side1 = nullptr, *side2 = nullptr;

        for (auto* child : CCArrayExt<CCNode*>(listLayer->getChildren())) {
            if (geode::cocos::isSpriteFrameName(child, "GJ_table_side_001.png")) {
                if (!side1) {
                    side1 = static_cast<CCSprite*>(child);
                } else if (child != side1) {
                    side2 = static_cast<CCSprite*>(child);
                    break;
                }
            }
        }

        if (top && bottom && side1 && side2) {
            auto top2 = CCSprite::createWithSpriteFrameName("list-border-top.png"_spr);
            top->setTexture(top2->getTexture());
            top->setTextureRect(top2->getTextureRect());

            auto bottom2 = CCSprite::createWithSpriteFrameName("list-border-bottom.png"_spr);
            bottom->setTexture(bottom2->getTexture());
            bottom->setTextureRect(bottom2->getTextureRect());

            for (auto* side : {side1, side2}) {
                auto id = side->getID();
                auto pos = side->getPosition();
                auto scaleY = side->getScaleY();

                side->removeFromParent();

                auto* spr = Build<CCSprite>::createSpriteName("list-border-side.png"_spr)
                    .zOrder(9)
                    .id(id)
                    .anchorPoint({0.f, 0.f})
                    .pos(pos)
                    .scaleY(scaleY)
                    .parent(listLayer)
                    .collect();

                if (side == side2) {
                    spr->setScaleX(-1.f);
                }
            }
        }

        listLayer->setColor({55, 55, 55});
    }

    void setCellColors(CCArray* cells, ccColor3B color) {
        for (auto cell : CCArrayExt<CCNode*>(cells)) {
            while (cell && !typeinfo_cast<GenericListCell*>(cell)) {
                cell = cell->getParent();
            }

            if (!cell) continue;

            static_cast<GenericListCell*>(cell)->m_backgroundLayer->setColor(color);
        }
    }

    CCSprite* makeRepeatingBackground(const char* texture, ccColor3B color, float xMult, float yMult, RepeatMode mode) {
        auto bg = CCSprite::create(texture);
        if (!bg) bg = CCSprite::createWithSpriteFrameName(texture);
        if (!bg) return nullptr;

        auto winSize = CCDirector::get()->getWinSize();

        auto bgrect = bg->getTextureRect();

        bool repeatX = mode == RepeatMode::X || mode == RepeatMode::Both;
        bool repeatY = mode == RepeatMode::Y || mode == RepeatMode::Both;
        if (repeatX) {
            bgrect.size.width = winSize.width * xMult;
        }

        if (repeatY) {
            bgrect.size.height = winSize.height * yMult;
        }

        ccTexParams tp = {
            GL_LINEAR, GL_LINEAR,
            static_cast<GLuint>(repeatX ? GL_REPEAT : GL_CLAMP_TO_EDGE),
            static_cast<GLuint>(repeatY ? GL_REPEAT : GL_CLAMP_TO_EDGE)
        };

        // TODO: dont overwrite global texture
        bg->getTexture()->setTexParameters(&tp);

        return Build(bg)
            .contentSize(winSize.width, winSize.height)
            .textureRect(bgrect)
            .zOrder(-1)
            .anchorPoint(0.f, 0.f)
            .color(color)
            .collect();
    }
}