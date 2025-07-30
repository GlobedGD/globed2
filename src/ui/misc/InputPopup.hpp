#pragma once

#include <Geode/ui/TextInput.hpp>
#include <ui/BasePopup.hpp>

namespace globed {

struct InputPopupOutcome {
    std::string text;
    bool cancelled = false;
};

class InputPopup : public BasePopup<InputPopup, const char*> {
public:
    static constexpr inline cocos2d::CCSize POPUP_SIZE = {};

    void setPlaceholder(const std::string& placeholder);
    void setFilter(const std::string& allowedChars);
    void setCommonFilter(geode::CommonFilter filter);
    void setMaxCharCount(size_t length);
    void setPasswordMode(bool password);
    void setDefaultText(const std::string& text);
    void setCallback(std::function<void(InputPopupOutcome)> callback);
    void setWidth(float width);
    void show() override;

protected:
    geode::Ref<geode::TextInput> m_input;
    CCMenuItemSpriteExtra* m_submitBtn;
    std::function<void(InputPopupOutcome)> m_callback;
    cocos2d::CCSize m_mySize{};
    size_t m_maxCharCount = 0;

    bool setup(const char* font) override;
    void onClose(cocos2d::CCObject*) override;
};

}
