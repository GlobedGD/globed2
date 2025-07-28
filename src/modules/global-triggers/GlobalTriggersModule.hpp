#pragma once

#include <globed/core/Core.hpp>
#include "CounterChange.hpp"

namespace globed {

class GlobalTriggersModule : public Module {
public:
    GlobalTriggersModule();

    virtual std::string_view name() const override {
        return "Global Triggers";
    }

    virtual std::string_view id() const override {
        return "globed.global-triggers";
    }

    virtual std::string_view author() const override {
        return "Globed";
    }

    virtual std::string_view description() const override {
        return "";
    }

    void onPlayerJoin(GlobedGJBGL* gjbgl, int accountId) override;
    void onPlayerLeave(GlobedGJBGL* gjbgl, int accountId) override;

    static GlobalTriggersModule& get();

    static void setInstance(std::shared_ptr<Module> instance) {
        g_instance = std::static_pointer_cast<GlobalTriggersModule>(std::move(instance));
    }

    void queueCounterChange(const CounterChange& change);

private:
    static inline std::shared_ptr<GlobalTriggersModule> g_instance;
};

}
