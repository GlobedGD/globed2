#include "BaseSettingCell.hpp"
#include <globed/core/PopupManager.hpp>

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

bool BaseSettingCellBase::init(
    CStr key,
    CStr name,
    CStr desc,
    cocos2d::CCSize size
) {
    m_key = key;
    return this->initNoSetting(name, desc, size);
}

bool BaseSettingCellBase::initNoSetting(CStr name, CStr desc, cocos2d::CCSize size) {
    CCMenu::init();

    this->ignoreAnchorPointForPosition(false);
    this->setCascadeColorEnabled(false);

    m_size = size;
    m_name = name;
    m_desc = desc;

    this->setContentSize(m_size);

    m_rightMenu = Build<CCMenu>::create()
        .layout(SimpleRowLayout::create()
            ->setMainAxisDirection(AxisDirection::RightToLeft)
            ->setMainAxisAlignment(MainAxisAlignment::Start)
            ->setMainAxisScaling(AxisScaling::Fit)
            ->setCrossAxisScaling(AxisScaling::Fit)
            ->setGap(3.f)
        )
        .parent(this)
        .anchorPoint(1.f, 0.5f)
        .posY(m_size.height / 2.f);
    m_rightMenu->setPositionX(m_size.width - 8.f);

    auto label = Build<CCLabelBMFont>::create(name, "bigFont.fnt")
        .limitLabelWidth(size.width * 0.65f, 0.5f, 0.25f)
        .anchorPoint(0.f, 0.5f)
        .pos(8.f, size.height / 2.f)
        .parent(this)
        .collect();

    if (!desc.empty()) {
        Build<CCSprite>::createSpriteName("GJ_infoIcon_001.png")
            .scale(0.45f)
            .intoMenuItem([this] {
                globed::alert(m_name, std::string(m_desc));
            })
            .pos(8.f + label->getScaledContentWidth() + 6.f, size.height / 2.f + label->getScaledContentHeight() / 2.f - 4.f)
            .parent(this);
    }

    this->setup();

    return true;
}

}