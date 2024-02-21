#include "color_input_popup.hpp"

#include <util/format.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;

bool GlobedColorInputPopup::setup(ccColor3B color, ColorInputCallbackFn fn) {
    this->callback = fn;
    this->storedColor = color;

    auto sizes = util::ui::getPopupLayout(m_size);

    colorPicker = CCControlColourPicker::colourPicker();
    colorPicker->setPosition(sizes.center + CCPoint{75.f, 0.f});
    colorPicker->setDelegate(this);
    m_mainLayer->addChild(colorPicker);

    auto* inputsRootLayout = Build<CCNode>::create()
        .layout(ColumnLayout::create()->setAxisReverse(true)->setGap(3.f))
        .pos(sizes.centerLeft + CCPoint{30.f, 20.f})
        .parent(m_mainLayer)
        .id("inputs-root-layout"_spr)
        .collect();

    // rgb
    auto* inputsRgbLayout = Build<CCNode>::create()
        .layout(RowLayout::create()->setGap(3.f))
        .parent(inputsRootLayout)
        .id("inputs-rgb-layout"_spr)
        .collect();

    Build<TextInput>::create(50.f, "")
        .scale(0.2f)
        .parent(inputsRgbLayout)
        .store(inputR);

    Build<TextInput>::create(50.f, "")
        .scale(0.2f)
        .parent(inputsRgbLayout)
        .store(inputG);

    Build<TextInput>::create(50.f, "")
        .scale(0.2f)
        .parent(inputsRgbLayout)
        .store(inputB);

    inputR->setCommonFilter(CommonFilter::Uint);
    inputR->setMaxCharCount(3);
    inputR->setCallback([this](auto text) {
        auto r = util::format::parse<uint8_t>(text);
        if (r.has_value()) {
            this->storedColor.r = r.value();
            this->updateColors(true);
        }
    });

    inputG->setCommonFilter(CommonFilter::Uint);
    inputG->setMaxCharCount(3);
    inputG->setCallback([this](auto text) {
        auto g = util::format::parse<uint8_t>(text);
        if (g.has_value()) {
            this->storedColor.g = g.value();
            this->updateColors(true);
        }
    });

    inputB->setCommonFilter(CommonFilter::Uint);
    inputB->setMaxCharCount(3);
    inputB->setCallback([this](auto text) {
        auto b = util::format::parse<uint8_t>(text);
        if (b.has_value()) {
            this->storedColor.b = b.value();
            this->updateColors(true);
        }
    });

    // i apologize for this hardcoded value but screw this
    inputsRgbLayout->setContentSize({156.f, 30.f});
    inputsRgbLayout->updateLayout();

    // hex
    Build<TextInput>::create(120.f, "")
        .scale(0.2f)
        .parent(inputsRootLayout)
        .store(inputHex);

    inputHex->setCommonFilter(CommonFilter::Hex);
    inputHex->setMaxCharCount(6);
    inputHex->setCallback([this](auto text) {
        if (text.size() != 6) {
            // let em cook
            return;
        }

        auto color = util::format::parseColor(text);
        if (color.isOk()) {
            this->storedColor = color.unwrap();
            this->updateColors(true);
        }
    });

    // this one too
    inputsRootLayout->setContentSize({0.f, 63.f});
    inputsRootLayout->updateLayout();

    // accept button
    Build<ButtonSprite>::create("Apply", "goldFont.fnt", "GJ_button_01.png", 0.8f)
        .intoMenuItem([this](auto) {
            this->callback(this->storedColor);
            this->onClose(this);
        })
        .pos(sizes.centerBottom + CCPoint{0.f, 20.f})
        .intoNewParent(CCMenu::create())
        .pos(0.f, 0.f)
        .parent(m_mainLayer);

    this->updateColors(true);

    return true;
}

void GlobedColorInputPopup::colorValueChanged(ccColor3B color) {
    storedColor = color;
    this->updateColors(false);
}

void GlobedColorInputPopup::updateColors(bool updateWheel) {
    if (updateWheel) {
        colorPicker->setColorValue(storedColor);
    }

    inputHex->setString(util::format::colorToHex(storedColor, false));

    inputR->setString(std::to_string(storedColor.r));
    inputG->setString(std::to_string(storedColor.g));
    inputB->setString(std::to_string(storedColor.b));
}

GlobedColorInputPopup* GlobedColorInputPopup::create(ccColor3B color, ColorInputCallbackFn fn) {
    auto ret = new GlobedColorInputPopup;
    if (ret->init(POPUP_WIDTH, POPUP_HEIGHT, color, fn)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}