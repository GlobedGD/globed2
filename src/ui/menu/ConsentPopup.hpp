#pragma once

#include <globed/util/CStr.hpp>
#include <ui/BasePopup.hpp>
#include <Geode/utils/function.hpp>

namespace globed {

using ConsentCallback = geode::Function<void(bool)>;

class ConsentPopup : public BasePopup {
public:
    static ConsentPopup* create();

    bool init();
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