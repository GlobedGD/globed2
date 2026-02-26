/// Core components for UI building
#pragma once

#include <AdvancedLabel.hpp>
#include <UIBuilder.hpp>
#include <cue/Slider.hpp>
#include <cue/ProgressBar.hpp>

namespace globed {

inline cue::Slider* createSlider() {
    return cue::Slider::create(
        "slider-start.png"_spr,
        "slider-middle.png"_spr,
        "slider-end.png"_spr,
        "slider-fill.png"_spr,
        "slider-thumb.png"_spr,
        "slider-thumb-sel.png"_spr
    );
}

inline cue::ProgressBar* createProgressBar() {
    return cue::ProgressBar::create(
        "slider-start.png"_spr,
        "slider-middle.png"_spr,
        "slider-end.png"_spr,
        "slider-fill.png"_spr
    );
}

/// Column container with auto grow, top to bottom
struct ColumnContainer : public cocos2d::CCMenu {
    static ColumnContainer* create(float gap = 3.f);
    ColumnContainer* flip();

    geode::SimpleColumnLayout* layout();
};

/// Row container with auto grow, left to right
struct RowContainer : public cocos2d::CCMenu {
    static RowContainer* create(float gap = 3.f);
    RowContainer* flip();

    geode::SimpleRowLayout* layout();
};

}
