#pragma once
#include <defs.hpp>

#include <data/types/gd.hpp>

class RemotePlayer;

class BaseVisualPlayer {
public:
    virtual bool init(RemotePlayer* parent, bool isSecond);
    virtual void updateIcons(const PlayerIconData& icons) = 0;
    virtual void updateName() = 0;
    virtual void updateData(const SpecificIconData& data, bool isDead, bool isPaused, bool isPracticing) = 0;
    virtual void updateIconType(PlayerIconType newType) = 0;
    virtual void playDeathEffect() = 0;
    virtual void playSpiderTeleport() = 0;

    static int getIconWithType(const PlayerIconData& data, PlayerIconType type);

protected:
    RemotePlayer* parent;
    bool isSecond;
};