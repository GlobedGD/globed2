#pragma once

#include <globed/prelude.hpp>
#include <ui/BasePopup.hpp>

namespace globed {

class UserActionsPopup : public BasePopup<UserActionsPopup, int, CCArray*> {
public:
    static const CCSize POPUP_SIZE;

private:
    Ref<CCMenu> m_buttons;
    int m_accountId;

    bool setup(int accountId, CCArray* buttons) override;
};

}