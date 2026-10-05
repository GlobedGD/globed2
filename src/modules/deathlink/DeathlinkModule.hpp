#pragma once

#include <globed/core/Module.hpp>

namespace globed {

class DeathlinkModule : public SoftModule<DeathlinkModule> {
public:
    DeathlinkModule();

    static inline const ModuleMetadata metadata {
        .id = "globed.deathlink",
        .name = "Deathlink",
        .author = "Globed",
    };

private:
    friend SoftModule;

    void onJoinLevel(GlobedGJBGL* gjbgl, GJGameLevel* level, bool editor) override;
    void onPlayerDeath(GlobedGJBGL* gjbgl, RemotePlayer* player, const PlayerDeath& death) override;
    bool shouldSpeedUpNewBest(GlobedGJBGL* gjbgl) override {
        return true;
    }

    bool wantsSyncReset() override {
        return true;
    }
};

}
