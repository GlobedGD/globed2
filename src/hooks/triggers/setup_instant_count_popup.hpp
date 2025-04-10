#pragma once

#include <defs/platform.hpp>

#ifdef GLOBED_GP_CHANGES

#include <Geode/modify/SetupInstantCountPopup.hpp>
#include <defs/minimal_geode.hpp>
#include <managers/hook.hpp>
#include <cocos2d.h>

struct GLOBED_DLL InstantCountPopupHook : geode::Modify<InstantCountPopupHook, SetupInstantCountPopup> {
    struct Fields {
        cocos2d::CCLabelBMFont* itemIdLabel = nullptr;
        CCTextInputNode* itemIdInputNode = nullptr;
        bool globedMode = false;
    };

    static void onModify(auto& self) {
        if (auto h = self.getHook("SetupInstantCountPopup::init")) {
            HookManager::get().addHook(HookManager::Group::EditorTriggerPopups, h.unwrap());
        }

        if (auto h = self.getHook("SetupInstantCountPopup::updateItemID")) {
            HookManager::get().addHook(HookManager::Group::EditorTriggerPopups, h.unwrap());
        }
    }

    $override
    bool init(CountTriggerGameObject* p0, cocos2d::CCArray* p1);

    $override
    void updateItemID();
};

#endif // GLOBED_GP_CHANGES
