#pragma once

#include <ui/BasePopup.hpp>

namespace globed {

class ModPanelPopup : public BasePopup<ModPanelPopup> {
public:
    static const cocos2d::CCSize POPUP_SIZE;

protected:
    geode::TextInput *m_queryInput;

    bool setup();
};

} // namespace globed
