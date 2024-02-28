#pragma once
#include <defs/all.hpp>

class AskInputPopup : public geode::Popup<
    const std::string_view, // title
    std::function<void(const std::string_view)>, // callback
    size_t, // chars
    const std::string_view, // placeholder
    const std::string_view // filter
    > {
public:
    static constexpr float POPUP_PADDING = 10.f;
    static constexpr float POPUP_HEIGHT = 140.f;

    static AskInputPopup* create(const std::string_view title, std::function<void(const std::string_view)> function, size_t chars, const std::string_view placeholder, const std::string_view filter, float widthMult = 1.f);

private:
    std::function<void(const std::string_view)> function;
    geode::InputNode* input = nullptr;

    bool setup(const std::string_view, std::function<void(const std::string_view)> function, size_t chars, const std::string_view placeholder, const std::string_view filter);
};
