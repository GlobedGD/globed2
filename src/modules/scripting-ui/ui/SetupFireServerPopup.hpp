#pragma once

#include <ui/BasePopup.hpp>
#include <modules/scripting/objects/FireServerObject.hpp>

#include <Geode/Geode.hpp>
#include <Geode/utils/function.hpp>

namespace globed {

class SetupFireServerPopup : public BasePopup {
public:
    static SetupFireServerPopup* create(FireServerObject* obj);

private:
    FireServerObject* m_object;
    FireServerPayload m_payload;
    CCMenuItemToggler* m_spawnTriggerBtn;
    CCMenuItemToggler* m_touchTriggerBtn;
    CCMenuItemToggler* m_multiTriggerBtn;
    CCNode* m_multiTriggerNode;
    bool m_invalidEventId = false;

    bool init(FireServerObject* obj);
    void onClose(CCObject*) override;

    CCNode* createParam(size_t idx, int value);
    CCNode* createInputBox(size_t idx, int value);

    void onPropChange(size_t idx, int value);
    void onArgTypeChange(size_t idx, FireServerArgType type);

    FireServerPayload getPayload();

    CCNode* addBaseCheckbox(const char* label, CCMenuItemToggler** store, geode::Function<void(CCMenuItemToggler*)> callback);
    void setTriggerState(bool spawn, bool touch);
};

}