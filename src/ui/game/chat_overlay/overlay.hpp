#pragma once
#include <Geode/Geode.hpp>

#include "overlay_cell.hpp"

class GlobedChatOverlay : public cocos2d::CCNode {
public:
    static GlobedChatOverlay* create();

    void addMessage(int accountId, const std::string& message);

private:
    std::vector<ChatOverlayCell*> overlayCells;

    bool init() override;
};
