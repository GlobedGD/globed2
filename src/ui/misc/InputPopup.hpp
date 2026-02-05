#pragma once

#include <Geode/ui/TextInput.hpp>
#include <Geode/utils/function.hpp>
#include <Geode/utils/ZStringView.hpp>
#include <ui/BasePopup.hpp>

namespace globed {

struct InputPopupOutcome {
    std::string text;
    bool cancelled = false;
};

class InputPopup : public BasePopup {
public:
    using BasePopup::setTitle; // publicize it

    static InputPopup* create(geode::ZStringView font = "chatFont.fnt");

    void setPlaceholder(const std::string& placeholder);
    void setFilter(const std::string& allowedChars);
    void setCommonFilter(geode::CommonFilter filter);
    void setMaxCharCount(size_t length);
    void setPasswordMode(bool password);
    void setDefaultText(const std::string& text);
    void setCallback(geode::Function<void(InputPopupOutcome)> callback);
    void setWidth(float width);
    void show() override;

protected:
    geode::Ref<geode::TextInput> m_input;
    CCMenuItemSpriteExtra* m_submitBtn;
    geode::Function<void(InputPopupOutcome)> m_callback;
    cocos2d::CCSize m_mySize{};
    std::string m_defaultText;
    size_t m_maxCharCount = 0;

    bool init(const char* font);
    void onClose(cocos2d::CCObject*) override;
};

}
