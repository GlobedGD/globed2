#pragma once

#include "base.hpp"

class DeathlinkModule : public BaseGameplayModule {
public:
    DeathlinkModule(GlobedGJBGL* gameLayer) : BaseGameplayModule(gameLayer) {}

    EventOutcome fullResetLevel() override;
    EventOutcome resetLevel() override;
    EventOutcome destroyPlayerPre(PlayerObject* player, GameObject* object) override;
    void destroyPlayerPost(PlayerObject* player, GameObject* object) override;
    void onUpdatePlayer(int playerId, RemotePlayer* player, const FrameFlags& flags) override;
    void selUpdate(float dt) override;
    void playerDestroyed(PlayerObject* player, bool unk) override;

private:
    bool oldFastReset = false;
    bool hasBeenKilled = false;

    void notifyDeath();
};