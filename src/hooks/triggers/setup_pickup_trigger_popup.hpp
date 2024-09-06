#pragma once

#include <Geode/modify/SetupPickupTriggerPopup.hpp>
#include <defs/minimal_geode.hpp>
#include <cocos2d.h>

struct GLOBED_DLL PickupPopupHook : geode::Modify<PickupPopupHook, SetupPickupTriggerPopup> {
    struct Fields {
        cocos2d::CCLabelBMFont* itemIdLabel = nullptr;
        CCTextInputNode* itemIdInputNode = nullptr;
        bool globedMode = false;
    };

    $override
    bool init(EffectGameObject* p0, cocos2d::CCArray* p1);

    $override
    void onPlusButton(cocos2d::CCObject*);

    void toggleGlobedMode(bool state, int itemId);

    void onUpdateValue(int p0, float p1);
};

