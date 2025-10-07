#pragma once

#include <globed/prelude.hpp>
#include <globed/core/data/FeaturedLevel.hpp>

namespace globed {

GJDifficultySprite* findDifficultySprite(CCNode* node);
void attachRatingSprite(CCNode* parent, FeatureTier tier);
void findAndAttachRatingSprite(CCNode* node, FeatureTier tier);
CCSprite* createRatingSprite(FeatureTier tier);

std::optional<FeatureTier> featureTierFromLevel(GJGameLevel* level);
void setFeatureTierForLevel(GJGameLevel* level, FeatureTier tier);

}