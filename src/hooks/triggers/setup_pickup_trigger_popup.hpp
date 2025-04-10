#pragma once

#include <defs/platform.hpp>

#ifdef GLOBED_GP_CHANGES

#include <Geode/modify/SetupPickupTriggerPopup.hpp>
#include <defs/minimal_geode.hpp>
#include <managers/hook.hpp>
#include <cocos2d.h>

struct GLOBED_DLL PickupPopupHook : geode::Modify<PickupPopupHook, SetupPickupTriggerPopup> {
    struct Fields {
        cocos2d::CCLabelBMFont* itemIdLabel = nullptr;
        CCTextInputNode* itemIdInputNode = nullptr;
        bool globedMode = false;
    };

    static void onModify(auto& self) {
        if (auto h = self.getHook("SetupPickupTriggerPopup::init")) {
            HookManager::get().addHook(HookManager::Group::EditorTriggerPopups, h.unwrap());
        }

        if (auto h = self.getHook("SetupPickupTriggerPopup::onPlusButton")) {
            HookManager::get().addHook(HookManager::Group::EditorTriggerPopups, h.unwrap());
        }
    }

    $override
    bool init(EffectGameObject* p0, cocos2d::CCArray* p1);

    $override
    void onPlusButton(cocos2d::CCObject*);

    void toggleGlobedMode(bool state, int itemId);

    void onUpdateValue(int p0, float p1);
};

#endif // GLOBED_GP_CHANGES
