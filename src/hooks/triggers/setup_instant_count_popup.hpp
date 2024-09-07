#pragma once

#include <Geode/modify/SetupInstantCountPopup.hpp>
#include <defs/minimal_geode.hpp>
#include <cocos2d.h>

struct GLOBED_DLL InstantCountPopupHook : geode::Modify<InstantCountPopupHook, SetupInstantCountPopup> {
    struct Fields {
        cocos2d::CCLabelBMFont* itemIdLabel = nullptr;
        CCTextInputNode* itemIdInputNode = nullptr;
        bool globedMode = false;
    };

    $override
    bool init(CountTriggerGameObject* p0, cocos2d::CCArray* p1);

    $override
    void updateItemID();
};

