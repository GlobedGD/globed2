#pragma once

#include <modules/scripting/objects/FireServerObject.hpp>
#include <ui/BasePopup.hpp>

#include <Geode/Geode.hpp>
#include <std23/move_only_function.h>

namespace globed {

class SetupFireServerPopup : public BasePopup<SetupFireServerPopup, FireServerObject *> {
    friend class BasePopup;
    static const cocos2d::CCSize POPUP_SIZE;

public:
    bool setup(FireServerObject *obj) override;

private:
    FireServerObject *m_object;
    FireServerPayload m_payload;
    CCMenuItemToggler *m_spawnTriggerBtn;
    CCMenuItemToggler *m_touchTriggerBtn;
    CCMenuItemToggler *m_multiTriggerBtn;
    CCNode *m_multiTriggerNode;
    bool m_invalidEventId = false;

    void onClose(CCObject *) override;

    CCNode *createParam(size_t idx, int value);
    CCNode *createInputBox(size_t idx, int value);

    void onPropChange(size_t idx, int value);
    void onArgTypeChange(size_t idx, FireServerArgType type);

    FireServerPayload getPayload();

    // TODO: in v5 change to move_only_function
    CCNode *addBaseCheckbox(const char *label, CCMenuItemToggler **store,
                            std::function<void(CCMenuItemToggler *)> callback);
    void setTriggerState(bool spawn, bool touch);
};

} // namespace globed