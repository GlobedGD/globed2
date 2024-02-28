#pragma once
#include <defs/all.hpp>

#include "setting_cell.hpp"
#include "setting_header_cell.hpp"

class GlobedSettingsLayer : public cocos2d::CCLayer {
public:
    static constexpr float LIST_WIDTH = 358.f;
    static constexpr float LIST_HEIGHT = 220.f;

    static GlobedSettingsLayer* create();

protected:
    GJListLayer* listLayer;

    bool init() override;
    void keyBackClicked() override;

    void remakeList();
    cocos2d::CCArray* createSettingsCells();

    template <typename T>
    constexpr GlobedSettingCell::Type getCellType() {
        using Type = GlobedSettingCell::Type;

        if constexpr (std::is_same_v<T, bool>) {
            return Type::Bool;
        } else if constexpr (std::is_same_v<T, float>) {
            return Type::Float;
        } else if constexpr (std::is_same_v<T, int>) {
            return Type::Int;
        } else {
            static_assert(std::is_same_v<T, void>, "invalid type for GlobedSettingLayer::getCellType");
        }
    }
};