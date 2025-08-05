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
};

}