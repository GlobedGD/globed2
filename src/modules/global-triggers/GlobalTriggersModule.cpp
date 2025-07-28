#include "GlobalTriggersModule.hpp"

#include <globed/core/Core.hpp>
#include <core/net/NetworkManagerImpl.hpp>
#include <core/hooks/GJBaseGameLayer.hpp>
#include "hooks/GJBaseGameLayer.hpp"
#include "Ids.hpp"

using namespace geode::prelude;

namespace globed {

GlobalTriggersModule::GlobalTriggersModule() {}

GlobalTriggersModule& GlobalTriggersModule::get() {
    if (!g_instance) {
        auto res = Core::get().addModule(GlobalTriggersModule{});

        if (!res) {
            geode::utils::terminate(fmt::format("Failed to initialize global triggers module: {}", res.unwrapErr()));
        }

        auto instance = res.unwrap();
        instance->setAutoEnableMode(AutoEnableMode::Level);

        GlobalTriggersModule::setInstance(instance);

        log::info("Global Triggers module initialized");
    }

    return *g_instance;
}

void GlobalTriggersModule::queueCounterChange(const CounterChange& change) {
    Event ev{};
    ev.type = EVENT_COUNTER_CHANGE;

    log::debug("Queuing counter change: type={}, itemId={}, value={}",
        (int)change.type, change.itemId, change.value.asInt());

    uint64_t rawData =
        (static_cast<uint64_t>(change.type) << 56)
        | ((static_cast<uint64_t>(change.itemId) & 0x00ffffffull) << 32);

    if (change.type == CounterChangeType::Set || change.type == CounterChangeType::Add) {
        rawData |= static_cast<uint64_t>((uint32_t) change.value.asInt());
    } else {
        rawData |= static_cast<uint64_t>(std::bit_cast<uint32_t>(change.value.asFloat()));
    }

    ev.data.resize(8);
    std::memcpy(ev.data.data(), &rawData, 8);

    NetworkManagerImpl::get().queueGameEvent(std::move(ev));
}

void GlobalTriggersModule::onPlayerJoin(GlobedGJBGL* gjbgl, int accountId) {
    GTriggersGJBGL::get(gjbgl)->handlePlayerJoin(accountId);
}

void GlobalTriggersModule::onPlayerLeave(GlobedGJBGL* gjbgl, int accountId) {
    GTriggersGJBGL::get(gjbgl)->handlePlayerLeave(accountId);
}

}

