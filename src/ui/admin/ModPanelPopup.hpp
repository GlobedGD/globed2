#pragma once

#include <ui/BasePopup.hpp>

namespace globed {

class ModPanelPopup : public BasePopup {
public:
    static ModPanelPopup* create();

protected:
    geode::TextInput* m_queryInput;

    bool init();
};

}
