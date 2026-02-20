#pragma once
#include <ui/BasePopup.hpp>
#include "Hooks.hpp"

namespace globed {

class APSSettingsPopup : public BasePopup {
public:
    static APSSettingsPopup* create(APSPlayLayer* layer);

private:
    APSSettings m_settings;
    APSPlayLayer* m_layer;
    MessageListener<msg::LevelDataMessage> m_listener;
    CCMenuItemSpriteExtra* m_btn;
    bool m_dirty = false;

    bool init(APSPlayLayer*);
    void apply();
    void startGame();
    void onClose(CCObject*);
};

}
