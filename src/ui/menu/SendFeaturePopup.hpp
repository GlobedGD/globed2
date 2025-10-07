#pragma once

#include <globed/prelude.hpp>
#include <globed/core/data/FeaturedLevel.hpp>
#include <ui/BasePopup.hpp>

namespace globed {

class SendFeaturePopup : public BasePopup<SendFeaturePopup, GJGameLevel*> {
public:
    static const CCSize POPUP_SIZE;

private:
    GJGameLevel* m_level;
    geode::TextInput* m_noteInput;
    CCMenu* m_menu;
    CCMenuItemSpriteExtra* m_diffButton = nullptr;

    FeatureTier m_chosenTier;

    bool setup(GJGameLevel* level);
    void createDiffButton();
    void sendLevel(bool queue);
};

}