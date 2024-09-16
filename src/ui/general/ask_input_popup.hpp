#pragma once
#include <defs/all.hpp>

class AskInputPopup : public geode::Popup<
    std::string_view, // title
    std::function<void(std::string_view)>, // callback
    size_t, // chars
    std::string_view, // placeholder
    std::string_view // filter
    > {
public:
    static constexpr float POPUP_PADDING = 10.f;
    static constexpr float POPUP_HEIGHT = 140.f;

    static AskInputPopup* create(std::string_view title, std::function<void(std::string_view)> function, size_t chars, std::string_view placeholder, std::string_view filter, float widthMult = 1.f);

private:
    std::function<void(std::string_view)> function;
    geode::InputNode* input = nullptr;

    bool setup(std::string_view, std::function<void(std::string_view)> function, size_t chars, std::string_view placeholder, std::string_view filter);
};
