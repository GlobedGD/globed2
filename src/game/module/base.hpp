#pragma once

#include <functional>
#include <string>

/* forward decls */

namespace cocos2d {
    class CCObject;
}

struct GlobedGJBGL;
struct FrameFlags;

class PlayerObject;
class GJGameLevel;
class GameObject;
class RemotePlayer;
class GlobedUserCell;
class GlobedPlayLayer;
class GlobedLevelEditorLayer;
class PlayerAccountData;

class BaseGameplayModule {
public:
    enum class [[nodiscard]] EventOutcome {
        Continue,
        Halt
    };

    BaseGameplayModule(GlobedGJBGL* gameLayer) : gameLayer(gameLayer) {}
    virtual ~BaseGameplayModule() {}

    virtual void onPlayerJoin(RemotePlayer* player) {}
    virtual void onPlayerLeave(RemotePlayer* player) {}

    // PlayerObject::update but only for player 1 and player 2
    virtual void mainPlayerUpdate(PlayerObject* player, float dt) {}

    // PlayerObject::update but for non-main players
    virtual void onlinePlayerUpdate(PlayerObject* player, float dt) {}

    virtual void loadLevelSettingsPre() {}
    virtual void loadLevelSettingsPost() {}

    virtual void checkCollisions(PlayerObject* player, float dt, bool p2) {}

    virtual EventOutcome fullResetLevel() { return EventOutcome::Continue; }
    virtual EventOutcome resetLevel() { return EventOutcome::Continue; }

    virtual void updateCameraPre(float dt) {}
    virtual void updateCameraPost(float dt) {}

    // fired when destroying player 1 or player 2
    virtual EventOutcome destroyPlayerPre(PlayerObject* player, GameObject* object) { return EventOutcome::Continue; }
    virtual void destroyPlayerPost(PlayerObject* player, GameObject* object) {}

    virtual void playerDestroyed(PlayerObject* player, bool unk) {}

    /* setup hooks */

    virtual void setupPreInit(GJGameLevel* level) {}
    virtual void setupBare() {}
    virtual void setupAudio() {}
    virtual void setupPacketListeners() {}
    virtual void setupCustomKeybinds() {}
    virtual void setupMisc() {}
    virtual void setupUi() {}
    virtual void postInitActions() {}

    /* selectors */

    virtual void selPeriodicalUpdate(float dt) {}
    virtual void selUpdate(float dt) {}
    virtual void selUpdateEstimators(float dt) {}
    virtual void onQuit() {}

    // called in selUpdate for each player
    virtual void onUpdatePlayer(int playerId, RemotePlayer* player, const FrameFlags& flags) {}

    virtual void onUnscheduleSelectors() {}
    virtual void onRescheduleSelectors(float timescale) {}

    virtual bool shouldSaveProgress() {
        return true;
    }

    /* UI stuff */

    struct UserCellButton {
        std::string spriteName;
        std::string id;
        std::function<void(cocos2d::CCObject*)> callback;
        int order = 0;
    };

    // Return a list of buttons to create.
    virtual std::vector<UserCellButton> onUserActionsPopup(int accountId, bool self) { return {}; }

protected:
    GlobedGJBGL* gameLayer;

    GlobedPlayLayer* getPlayLayer();
    GlobedLevelEditorLayer* getEditorLayer();
};