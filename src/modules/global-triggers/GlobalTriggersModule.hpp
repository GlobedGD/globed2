#pragma once

#include <globed/core/Module.hpp>
#include "CounterChange.hpp"

namespace globed {

class GlobalTriggersModule : public SoftModule<GlobalTriggersModule> {
public:
    GlobalTriggersModule();

    static constexpr inline auto AUTO_ENABLE = AutoEnableMode::Level;

    static inline const ModuleMetadata metadata {
        .id = "globed.global-triggers",
        .name = "Global Triggers",
        .author = "Globed",
    };

    void onPlayerJoin(GlobedGJBGL* gjbgl, int accountId) override;
    void onPlayerLeave(GlobedGJBGL* gjbgl, int accountId) override;

    void queueCounterChange(const CounterChange& change);
};

}
