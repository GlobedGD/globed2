#pragma once

#include <globed/util/CStr.hpp>
#include <ui/BasePopup.hpp>
#include <std23/move_only_function.h>

namespace globed {

using ConsentCallback = std23::move_only_function<void(bool)>;

class ConsentPopup : public BasePopup<ConsentPopup> {
public:
    static const cocos2d::CCSize POPUP_SIZE;

    bool setup();
    void setCallback(ConsentCallback&& cb);

private:
    ConsentCallback m_callback;
    CCMenuItemToggler* m_vcButton = nullptr;
    CCMenuItemToggler* m_qcButton = nullptr;
    CCMenuItemSpriteExtra* m_acceptBtn;
    bool m_rulesAccepted = false;
    bool m_privacyAccepted = false;

    void onButton(bool accept);
    CCNode* createClause(CStr description, CStr popupTitle, std::string_view content, bool* accepted);
    void updateAcceptButton();
};

}