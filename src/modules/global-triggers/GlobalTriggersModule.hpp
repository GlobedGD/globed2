#pragma once

#include "CounterChange.hpp"
#include <globed/core/ModuleCrtp.hpp>

namespace globed {

class GlobalTriggersModule : public ModuleCrtpBase<GlobalTriggersModule> {
public:
    GlobalTriggersModule();

    void onModuleInit();

    virtual std::string_view name() const override
    {
        return "Global Triggers";
    }

    virtual std::string_view id() const override
    {
        return "globed.global-triggers";
    }

    virtual std::string_view author() const override
    {
        return "Globed";
    }

    virtual std::string_view description() const override
    {
        return "";
    }

    void onPlayerJoin(GlobedGJBGL *gjbgl, int accountId) override;
    void onPlayerLeave(GlobedGJBGL *gjbgl, int accountId) override;

    void queueCounterChange(const CounterChange &change);

private:
};

} // namespace globed
