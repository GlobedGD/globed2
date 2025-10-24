#include "ButtonSettingCell.hpp"

#include <UIBuilder.hpp>
#include <cue/Util.hpp>

using namespace geode::prelude;

namespace globed {

void ButtonSettingCell::setup() {
    auto btn = Build<ButtonSprite>::create(m_btnText, "goldFont.fnt", "GJ_button_04.png", 0.8f)
        .scale(0.8f)
        .intoMenuItem([this] {
            m_callback();
        })
        .parent(this)
        .posY(m_size.height / 2.f)
        .collect();

    btn->setPositionX(m_size.width - 8.f - btn->getContentWidth() / 2.f);
}

ButtonSettingCell* ButtonSettingCell::create(
    CStr name,
    CStr desc,
    CStr btnText,
    Callback&& cb,
    cocos2d::CCSize cellSize
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