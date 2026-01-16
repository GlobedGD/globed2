#pragma once

#include <cue/ProgressBar.hpp>
#include <cue/Slider.hpp>

namespace globed {

inline cue::Slider *createSlider()
{
    return cue::Slider::create("slider-start.png"_spr, "slider-middle.png"_spr, "slider-end.png"_spr,
                               "slider-fill.png"_spr, "slider-thumb.png"_spr, "slider-thumb-sel.png"_spr);
}

inline cue::ProgressBar *createProgressBar()
{
    return cue::ProgressBar::create("slider-start.png"_spr, "slider-middle.png"_spr, "slider-end.png"_spr,
                                    "slider-fill.png"_spr);
}

} // namespace globed