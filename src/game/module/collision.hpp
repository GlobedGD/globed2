#pragma once

#include "base.hpp"

class CollisionModule : public BaseGameplayModule {
public:
    CollisionModule(GlobedGJBGL* gameLayer) : BaseGameplayModule(gameLayer) {}

    void loadLevelSettingsPre() override;
    void loadLevelSettingsPost() override;
    void checkCollisions(PlayerObject* player, float dt, bool p2) override;

private:
    bool lastPlat = false;
    int lastLength = 0;
};