#pragma once
#include <defs/all.hpp>
#include <Geode/ui/TextInput.hpp>

class AskInputPopup : public geode::Popup<
    std::string_view, // title
    std::function<void(std::string_view, bool)>, // callback
    size_t, // chars
    std::string_view, // placeholder
    std::string_view, // filter
    std::string_view  // checkbox text (optional)
    > {
public:
    static constexpr float POPUP_PADDING = 10.f;
    static constexpr float POPUP_HEIGHT = 140.f;

    static AskInputPopup* create(
        std::string_view title,
        std::function<void(std::string_view)> function,
        size_t chars,
        std::string_view placeholder,
        std::string_view filter,
        float widthMult = 1.f,
        std::string_view checkboxText = ""
    );

    static AskInputPopup* create(
        std::string_view title,
        std::function<void(std::string_view, bool)> function,
        size_t chars,
        std::string_view placeholder,
        std::string_view filter,
        float widthMult = 1.f,
        std::string_view checkboxText = ""
    );

private:
    std::function<void(std::string_view, bool)> function;
    geode::TextInput* input = nullptr;
    CCMenuItemToggler* checkbox = nullptr;

    bool setup(std::string_view, std::function<void(std::string_view, bool)> function, size_t chars, std::string_view placeholder, std::string_view filter, std::string_view checkboxText);
};
