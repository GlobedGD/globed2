#pragma once
#include <defs/all.hpp>

#include <data/types/gd.hpp>
#include <game/player_store.hpp>
#include <ui/general/audio_visualizer.hpp>

using namespace geode::prelude;

class GlobedUserChatCell : public CCLayerColor {
    public:
        std::string user;
        int accountId;

        void onUser(CCObject* sender);
        bool init(std::string username, int accid, std::string messageText);
        static GlobedUserChatCell* create(std::string username, int aid, std::string messageText);

};