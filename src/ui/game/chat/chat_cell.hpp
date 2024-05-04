#pragma once
#include <defs/all.hpp>

#include <data/types/gd.hpp>
#include <game/player_store.hpp>
#include <ui/general/audio_visualizer.hpp>

class GlobedChatCell : public cocos2d::CCLayerColor {
public:
    static constexpr float CELL_HEIGHT = 44.f;

    std::string user;
    int accountId;

    void onUser(cocos2d::CCObject* sender);
    bool init(const std::string& username, int accid, const std::string& messageText);
    static GlobedChatCell* create(const std::string& username, int aid, const std::string& messageText);
};