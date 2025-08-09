#pragma once

#include <ui/BasePopup.hpp>

namespace globed {

class ModAuditLogPopup : public BasePopup<ModAuditLogPopup> {
public:
    static const cocos2d::CCSize POPUP_SIZE;

protected:
    bool setup();
};

}