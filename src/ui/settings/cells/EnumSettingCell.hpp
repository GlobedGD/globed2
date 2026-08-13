#pragma once
#include "ButtonSettingCell.hpp"

namespace globed {

class EnumSettingCell : public ButtonSettingCell {
public:
    static EnumSettingCell* create(
        ZStringView key,
        ZStringView name,
        ZStringView desc,
        std::vector<std::pair<ZStringView, int>> options,
        cocos2d::CCSize cellSize
    );

    template <typename E>
    static EnumSettingCell* create(
        ZStringView key,
        ZStringView name,
        ZStringView desc,
        std::vector<std::pair<ZStringView, E>> options,
        cocos2d::CCSize cellSize
    ) {
        std::vector<std::pair<ZStringView, int>> intOptions;
        intOptions.reserve(options.size());
        for (auto& option : options) {
            intOptions.emplace_back(option.first, static_cast<int>(option.second));
        }
        return create(key, name, desc, std::move(intOptions), cellSize);
    }

protected:
    std::vector<std::pair<ZStringView, int>> m_options;
    int m_current = -1;
    size_t m_currentIdx = 0;

    void setup() override;
    void reload() override;
    void pickNext();
};

}