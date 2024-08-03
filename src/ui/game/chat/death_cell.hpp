#pragma once
#include <defs/all.hpp>

#include <data/types/gd.hpp>
#include <game/player_store.hpp>

class GlobedDeathCell : public cocos2d::CCLayerColor {
public:
    static constexpr float CELL_HEIGHT = 24.f;
    static constexpr float CELL_WIDTH = 410.f;

    std::string user;
    int accountId;
    float timer = 0.f; // fire emoji

    void onUser(cocos2d::CCObject* sender);
    void updateTimer(float dt);
    bool init(const std::string& username, int accid);
    static GlobedDeathCell* create(const std::string& username, int aid);
};