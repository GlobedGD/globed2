#pragma once

#include <globed/core/ModuleCrtp.hpp>

namespace globed {

class DeathlinkModule : public ModuleCrtpBase<DeathlinkModule> {
public:
    DeathlinkModule();

    void onModuleInit();

    virtual std::string_view name() const override {
        return "Deathlink";
    }

    virtual std::string_view id() const override {
        return "globed.deathlink";
    }

    virtual std::string_view author() const override {
        return "Globed";
    }

    virtual std::string_view description() const override {
        return "";
    }

private:
    void onJoinLevel(GlobedGJBGL* gjbgl, GJGameLevel* level, bool editor) override;
    void onPlayerDeath(GlobedGJBGL* gjbgl, RemotePlayer* player, const PlayerDeath& death) override;
    bool shouldSpeedUpNewBest(GlobedGJBGL* gjbgl) override {
        return true;
    }
};

}
