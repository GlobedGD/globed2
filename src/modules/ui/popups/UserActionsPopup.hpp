#pragma once

#include <globed/prelude.hpp>
#include <ui/BasePopup.hpp>

namespace globed {

class UserActionsPopup : public BasePopup {
public:
    static UserActionsPopup* create(int accountId, std::vector<CCNode*> buttons);

private:
    Ref<CCMenu> m_buttons;
    int m_accountId;

    bool init(int accountId, std::vector<CCNode*> buttons);
};

}