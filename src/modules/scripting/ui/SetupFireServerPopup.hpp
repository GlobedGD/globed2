#pragma once

#include <ui/BasePopup.hpp>

#include <Geode/Geode.hpp>

namespace globed {

class SetupFireServerPopup : public BasePopup<SetupFireServerPopup, ItemTriggerGameObject*> {
    friend class BasePopup;
    static const cocos2d::CCSize POPUP_SIZE;

public:
    bool setup(ItemTriggerGameObject* obj) override;
};

}