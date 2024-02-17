#include "ask_input_popup.hpp"

using namespace geode::prelude;

bool AskInputPopup::setup(const std::string_view title, std::function<void(const std::string_view)> function, size_t chars, const std::string_view placeholder, const std::string_view filter) {
    this->setTitle(std::string(title));
    this->function = function;

    auto winSize = CCDirector::get()->getWinSize();

    Build<InputNode>::create(m_size.width - POPUP_PADDING * 2.f, std::string(placeholder).c_str(), "chatFont.fnt", std::string(filter), chars)
        .pos(winSize / 2 + CCPoint{0.f, 10.f})
        .parent(m_mainLayer)
        .id("generic-popup-input"_spr)
        .store(input);

    Build<ButtonSprite>::create("Submit", "bigFont.fnt", "GJ_button_01.png", 0.4f)
        .intoMenuItem([this](auto) {
            this->onClose(this);
            this->function(this->input->getString());
        })
        .pos(winSize / 2 + CCPoint{0.f, -40.f})
        .intoNewParent(CCMenu::create())
        .pos(0.f, 0.f)
        .parent(m_mainLayer);

    return true;
}

AskInputPopup* AskInputPopup::create(const std::string_view title, std::function<void(const std::string_view)> function, size_t chars, const std::string_view placeholder, const std::string_view filter, float widthMult) {
    auto ret = new AskInputPopup;
    if (ret->init(POPUP_PADDING * 2 + 4.f * chars * widthMult, POPUP_HEIGHT, title, function, chars, placeholder, filter)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}