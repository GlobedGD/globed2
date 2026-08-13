#include "ButtonSettingCell.hpp"

#include <UIBuilder.hpp>
#include <cue/Util.hpp>

using namespace geode::prelude;

namespace globed {

void ButtonSettingCell::setup() {
    this->createButton(m_btnText);
}

void ButtonSettingCell::createButton(ZStringView text) {
    cue::resetNode(m_button);

    m_button = Build<ButtonSprite>::create(m_btnText.c_str(), "goldFont.fnt", "GJ_button_04.png", 0.8f)
        .scale(0.7f)
        .intoMenuItem([this] {
            m_callback();
        })
        .scaleMult(1.15f)
        .parent(m_rightMenu)
        .collect();
    m_rightMenu->updateLayout();

}

ButtonSettingCell* ButtonSettingCell::create(
    ZStringView name,
    ZStringView desc,
    ZStringView btnText,
    Callback&& cb,
    CCSize cellSize
) {
    auto ret = new ButtonSettingCell;
    ret->m_callback = std::move(cb);
    ret->m_btnText = btnText;

    if (ret->initNoSetting(name, desc, cellSize)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

}