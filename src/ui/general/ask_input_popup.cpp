#include "ask_input_popup.hpp"

#include <util/ui.hpp>

using namespace geode::prelude;

bool AskInputPopup::setup(std::string_view title, std::function<void(std::string_view, bool)> function, size_t chars, std::string_view placeholder, std::string_view filter, std::string_view checkboxText) {
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

    CCMenu* btnMenu;

    Build<ButtonSprite>::create("Submit", "bigFont.fnt", "GJ_button_01.png", 0.7f)
        .intoMenuItem([this](auto) {
            this->onClose(this);

            bool cb = this->checkbox ? this->checkbox->isOn() : false;
            this->function(std::string_view(this->input->getString()), cb);
        })
        .intoNewParent(CCMenu::create())
        .store(btnMenu)
        .layout(RowLayout::create())
        .pos(rlayout.fromCenter(0.f, -40.f))
        .contentSize(m_size.width, 40.f)
        .parent(m_mainLayer);

    if (!checkboxText.empty()) {
        // add a checkbox with text

        Build(CCMenuItemExt::createTogglerWithStandardSprites(0.7f, [this](auto btn) {}))
            .parent(btnMenu)
            .store(checkbox);

        Build<CCLabelBMFont>::create(std::string(checkboxText).c_str(), "bigFont.fnt")
            .with([&](CCLabelBMFont* lbl) {
                lbl->setLayoutOptions(AxisLayoutOptions::create()->setAutoScale(false));
            })
            .limitLabelWidth(100.f,0.4f, 0.1f)
            .parent(btnMenu);
    }

    btnMenu->updateLayout();

    return true;
}

AskInputPopup* AskInputPopup::create(
    std::string_view title,
    std::function<void(std::string_view, bool)> function,
    size_t chars,
    std::string_view placeholder,
    std::string_view filter,
    float widthMult,
    std::string_view checkboxText
) {
    auto ret = new AskInputPopup;
    if (ret->initAnchored(POPUP_PADDING * 2 + 4.f * chars * widthMult, POPUP_HEIGHT, title, function, chars, placeholder, filter, checkboxText)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

AskInputPopup* AskInputPopup::create(
    std::string_view title,
    std::function<void(std::string_view)> function,
    size_t chars,
    std::string_view placeholder,
    std::string_view filter,
    float widthMult,
    std::string_view checkboxText
) {
    auto func = [f = std::move(function)](std::string_view a, bool b) {
        f(a);
    };

    return create(title, func, chars, placeholder, filter, widthMult, checkboxText);
}
