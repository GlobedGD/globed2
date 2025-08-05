#pragma once

#include <ui/BasePopup.hpp>
#include <modules/scripting/objects/FireServerObject.hpp>

#include <Geode/Geode.hpp>

namespace globed {

class SetupFireServerPopup : public BasePopup<SetupFireServerPopup, FireServerObject*> {
    friend class BasePopup;
    static const cocos2d::CCSize POPUP_SIZE;

public:
    bool setup(FireServerObject* obj) override;

private:
    FireServerObject* m_object;
    FireServerPayload m_payload;
    CCMenuItemToggler* m_spawnTriggerBtn;
    CCMenuItemToggler* m_touchTriggerBtn;
    CCMenuItemToggler* m_multiTriggerBtn;
    CCNode* m_multiTriggerNode;

    void onClose(CCObject*) override;

    CCNode* createParam(size_t idx, int value);
    CCNode* createInputBox(size_t idx, int value);

    void onPropChange(size_t idx, int value);
    void onArgTypeChange(size_t idx, FireServerArgType type);

    FireServerPayload getPayload();

    CCNode* addBaseCheckbox(const char* label, CCMenuItemToggler** store, std::function<void(CCMenuItemToggler*)> callback);
    void setTriggerState(bool spawn, bool touch);
};

}