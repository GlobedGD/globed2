#include "IntCornerSettingCell.hpp"

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

void IntCornerSettingCell::setup() {
    // this is a very fucked up thing

    m_innerSquare = Build(CCLayerColor::create({ 89, 172, 255, 255 }, 12.f, 12.f))
        .ignoreAnchorPointForPos(false);

    auto container = Build<CCNode>::create()
        .anchorPoint(0.5f, 0.5f)
        .contentSize(32.f, 32.f)
        .child(m_innerSquare)
        .collect();

    auto btn = Build(EditorButtonSprite::create(container, EditorBaseColor::Gray))
        .scale(0.85f)
        .intoMenuItem([this](auto) {
            int newVal = this->get<int>() + 1;
            if (newVal > 3 || newVal < 0) {
                newVal = 0;
            }
            this->set(newVal);

            this->reload();
        })
        .parent(this)
        .posY(m_size.height / 2.f)
        .collect();

    btn->setPositionX(m_size.width - 8.f - btn->getContentWidth() / 2.f);

    this->reload();
}

void IntCornerSettingCell::reload() {
    int val = this->get<int>();

    float sqw = 16.f;

    float y = sqw + (val < 2 ? 1 : -1) * sqw * 0.5f;
    float x = sqw + (val % 2 == 0 ? -1 : 1) * sqw * 0.5f;

    m_innerSquare->setPosition({ x, y });
}

}
