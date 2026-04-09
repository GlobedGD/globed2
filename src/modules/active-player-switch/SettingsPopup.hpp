#pragma once
#include <ui/BasePopup.hpp>
#include "Hooks.hpp"

namespace globed {

class APSSettingsPopup : public BasePopup {
public:
    static APSSettingsPopup* create();

private:
    APSSettings m_settings;
    MessageListener<msg::LevelDataMessage> m_listener;
    CCMenuItemSpriteExtra* m_btn;
    bool m_dirty = false;

    bool init();
    void apply();
    void startGame();
    void onClose(CCObject*);
};

}
