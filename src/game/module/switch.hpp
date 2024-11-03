#pragma once

#include "base.hpp"
#include <defs/platform.hpp>

class GLOBED_DLL SwitchModule : public BaseGameplayModule {
public:
    SwitchModule(GlobedGJBGL* gameLayer);

    EventOutcome resetLevel() override;

    void selUpdate(float dt) override;
    void mainPlayerUpdate(PlayerObject* player, float dt) override;

private:
    int linkedTo = 0;
    static inline const std::string LOCKED_TO_KEY = "sw-locked-to"_spr;

    void updateFromLockedPlayer(PlayerObject* player);
    void linkPlayerTo(int accountId);
    void unlink();
};