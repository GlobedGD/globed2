#pragma once

#include <globed/core/Module.hpp>

namespace globed {

class APSModule : public SoftModule<APSModule> {
public:
    APSModule() {}

    static constexpr inline auto AUTO_ENABLE = AutoEnableMode::Level;

    static inline const ModuleMetadata metadata {
        .id = "globed.switcheroo",
        .name = "Switcheroo",
        .author = "Globed",
    };

private:
    friend SoftModule;

    void onJoinLevel(GlobedGJBGL* gjbgl, GJGameLevel* level, bool editor) override;
    void onPlayerDeath(GlobedGJBGL* gjbgl, RemotePlayer* player, const PlayerDeath& death) override;
    void onUpdate(GlobedGJBGL* gjbgl, float dt) override;
    void onLocalPlayerDeath(GlobedGJBGL* gjbgl, bool real) override;
    void onPlayerRespawn(GlobedGJBGL* gjbgl, RemotePlayer* player) override;
    bool shouldSpeedUpNewBest(GlobedGJBGL* gjbgl) override {
        return true;
    }
    bool wantsSyncReset() override {
        return true;
    }
};

}
