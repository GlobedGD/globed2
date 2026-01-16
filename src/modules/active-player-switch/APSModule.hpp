#pragma once

#include <globed/core/ModuleCrtp.hpp>

namespace globed {

class APSModule : public ModuleCrtpBase<APSModule> {
public:
    APSModule();

    void onModuleInit();

    virtual std::string_view name() const override
    {
        return "Active Player Switch";
    }

    virtual std::string_view id() const override
    {
        return "globed.active-player-switch";
    }

    virtual std::string_view author() const override
    {
        return "Globed";
    }

    virtual std::string_view description() const override
    {
        return "";
    }

private:
    void onJoinLevel(GlobedGJBGL *gjbgl, GJGameLevel *level, bool editor) override;
    void onPlayerDeath(GlobedGJBGL *gjbgl, RemotePlayer *player, const PlayerDeath &death) override;
};

} // namespace globed
