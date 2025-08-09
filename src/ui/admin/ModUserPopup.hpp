#pragma once

#include <ui/BasePopup.hpp>

namespace globed {

class ModUserPopup : public BasePopup<ModUserPopup, int> {
public:
    static const cocos2d::CCSize POPUP_SIZE;

protected:
    bool setup(int accountId);
};

}
