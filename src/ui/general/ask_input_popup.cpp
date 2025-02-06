#include "ask_input_popup.hpp"

#include <util/ui.hpp>

using namespace geode::prelude;

bool AskInputPopup::setup(std::string_view title, std::function<void(std::string_view)> function, size_t chars, std::string_view placeholder, std::string_view filter) {
    this->setTitle(std::string(title));
    this->function = function;

    auto rlayout = util::ui::getPopupLayoutAnchored(m_size);

    Build<TextInput>::create(m_size.width - POPUP_PADDING * 2.f, std::string(placeholder).c_str(), "chatFont.fnt")
        .pos(rlayout.fromCenter(0.f, 10.f))
        .parent(m_mainLayer)
        .id("generic-popup-input"_spr)
        .store(input);

    input->setFilter(std::string(filter));
    input->setMaxCharCount(chars);

    Build<ButtonSprite>::create("Submit", "bigFont.fnt", "GJ_button_01.png", 0.7f)
        .intoMenuItem([this](auto) {
            this->onClose(this);
            this->function(this->input->getString());
        })
        .pos(rlayout.fromCenter(0.f, -40.f))
        .intoNewParent(CCMenu::create())
        .pos(0.f, 0.f)
        .parent(m_mainLayer);

    return true;
}

AskInputPopup* AskInputPopup::create(std::string_view title, std::function<void(std::string_view)> function, size_t chars, std::string_view placeholder, std::string_view filter, float widthMult) {
    auto ret = new AskInputPopup;
    if (ret->initAnchored(POPUP_PADDING * 2 + 4.f * chars * widthMult, POPUP_HEIGHT, title, function, chars, placeholder, filter)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}