#pragma once

#include <globed/core/ModuleCrtp.hpp>

namespace globed {

class CollisionModule : public ModuleCrtpBase<CollisionModule> {
public:
    CollisionModule();

    void onModuleInit();

    virtual std::string_view name() const override
    {
        return "Collision";
    }

    virtual std::string_view id() const override
    {
        return "globed.collision";
    }

    virtual std::string_view author() const override
    {
        return "Globed";
    }

    virtual std::string_view description() const override
    {
        return "";
    }

    void checkCollisions(GlobedGJBGL *gjbgl, PlayerObject *player, float dt, bool p2);

private:
    void onJoinLevel(GlobedGJBGL *gjbgl, GJGameLevel *level, bool editor) override;
};

} // namespace globed