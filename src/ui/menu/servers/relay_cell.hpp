#pragma once

#include <defs/geode.hpp>
#include <data/types/misc.hpp>

class RelaySwitchPopup;

class RelayCell : public cocos2d::CCLayer {
public:
    static constexpr float CELL_HEIGHT = 40.f;

    static RelayCell* create(const ServerRelay& relay, RelaySwitchPopup* parent);
    void refresh();

private:
    ServerRelay m_data;
    RelaySwitchPopup* m_parent;
    CCMenuItemSpriteExtra* m_btnSelect;
    CCMenuItemSpriteExtra* m_btnSelected;

    bool init(const ServerRelay& relay, RelaySwitchPopup* parent);
};
