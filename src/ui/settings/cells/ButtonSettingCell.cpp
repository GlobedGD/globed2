#include "ButtonSettingCell.hpp"

#include <UIBuilder.hpp>
#include <cue/Util.hpp>

using namespace geode::prelude;

namespace globed {

void ButtonSettingCell::setup() {
    this->createButton(m_btnText);
}

void ButtonSettingCell::createButton(CStr text) {
    cue::resetNode(m_button);

    m_button = Build<ButtonSprite>::create(m_btnText, "goldFont.fnt", "GJ_button_04.png", 0.8f)
        .scale(0.8f)
        .intoMenuItem([this] {
            m_callback();
        })
        .scaleMult(1.15f)
        .parent(this)
        .posY(m_size.height / 2.f)
        .collect();

    m_button->setPositionX(m_size.width - 8.f - m_button->getContentWidth() / 2.f);
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