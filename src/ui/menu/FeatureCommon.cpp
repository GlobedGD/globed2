#include "FeatureCommon.hpp"

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

GJDifficultySprite* findDifficultySprite(CCNode* node) {
    if (auto lc = typeinfo_cast<LevelCell*>(node)) {
        auto* diff = typeinfo_cast<GJDifficultySprite*>(lc->m_mainLayer->getChildByIDRecursive("difficulty-sprite"));
        if (diff) return diff;

        for (auto* child : CCArrayExt<CCNode*>(lc->m_mainLayer->getChildren())) {
            if (auto p = child->getChildByType<GJDifficultySprite>(0)) {
                return p;
            }
        }
    } else if (auto ll = typeinfo_cast<LevelInfoLayer*>(node)) {
        auto* diff = typeinfo_cast<GJDifficultySprite*>(ll->getChildByIDRecursive("difficulty-sprite"));
        if (diff) return diff;

        return ll->getChildByType<GJDifficultySprite>(0);
    }

    return nullptr;
}

static void attachOverlayToSprite(CCNode* parent) {
    Build<CCSprite>::create("icon-outstanding-overlay.png"_spr)
        .pos(parent->getScaledContentSize() / 2)
        .blendFunc({GL_ONE, GL_ONE})
        .color({200, 255, 255})
        .opacity(175)
        .zOrder(1)
        .parent(parent);
}

void attachRatingSprite(CCNode* parent, FeatureTier tier) {
    if (!parent) return;

    if (parent->getChildByID("globed-rating"_spr)) {
        return;
    }

    for (CCNode* child : CCArrayExt<CCNode*>(parent->getChildren())) {
        child->setVisible(false);
    }

    auto spr = createRatingSprite(tier);
    spr->setPosition(parent->getScaledContentSize() / 2);

    if (tier == FeatureTier::Outstanding) {
        attachOverlayToSprite(parent);
    }

    parent->addChild(spr);
}

void findAndAttachRatingSprite(CCNode* node, FeatureTier tier) {
    if (auto diff = findDifficultySprite(node)) {
        attachRatingSprite(diff, tier);
    }
}

CCSprite* createRatingSprite(FeatureTier tier) {
    CCSprite* spr;
    switch (tier) {
        case FeatureTier::Epic:
            spr = CCSprite::create("icon-epic.png"_spr);
            break;
        case FeatureTier::Outstanding:
            spr = CCSprite::create("icon-outstanding.png"_spr);
            break;
        case FeatureTier::Normal:
        default:
            spr = CCSprite::create("icon-featured.png"_spr);
            break;
    }

    spr->setZOrder(-1);
    spr->setID("globed-rating"_spr);

    if (tier == FeatureTier::Outstanding) {
        auto particle = GameToolbox::particleFromString("26a-1a1.25a0.3a16a90a62a4a0a20a20a0a16a0a0a0a0a4a2a0a0a0.341176a0a1a0a0.635294a0a1a0a0a1a0a0a0.247059a0a1a0a0.498039a0a1a0a0.16a0a0.23a0a0a0a0a0a0a0a0a2a1a0a0a0a0a0a0a0a0a0a0a0a0a0a0a0a0a0a0", nullptr, false);
        particle->setPosition(spr->getScaledContentSize() / 2 + CCPoint{0.f, 4.f});
        particle->setZOrder(-2);
        spr->addChild(particle);
    }

    return spr;
}

}