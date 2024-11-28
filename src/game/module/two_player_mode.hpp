#pragma once

#include "base.hpp"
#include <Geode/loader/Mod.hpp> // for _spr
#include <defs/platform.hpp>

class GLOBED_DLL TwoPlayerModeModule : public BaseGameplayModule {
public:
    TwoPlayerModeModule(GlobedGJBGL* gameLayer);

    void mainPlayerUpdate(PlayerObject* player, float dt) override;
    EventOutcome resetLevel() override;
    EventOutcome destroyPlayerPre(PlayerObject* player, GameObject* object) override;
    void destroyPlayerPost(PlayerObject* player, GameObject* object) override;

    std::vector<UserCellButton> onUserActionsPopup(int accountId, bool self) override;
    bool onUnpause() override;
    bool shouldSaveProgress() override;

    void selPeriodicalUpdate(float dt) override;

    int getLinkedTo();

private:
    bool isPrimary = false;
    bool oldFastReset = false;
    bool linked = false;
    int linkedId = 0;

    static inline const std::string LOCKED_TO_KEY = "2p-locked-to"_spr;

    void updateFromLockedPlayer(PlayerObject* player, bool ignorePos);
    void linkPlayerTo(int accountId);

    void unlinkIfAlone();
    void unlink();
};