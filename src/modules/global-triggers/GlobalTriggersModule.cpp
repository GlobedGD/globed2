#include "GlobalTriggersModule.hpp"

#include "hooks/GJBaseGameLayer.hpp"
#include <core/hooks/GJBaseGameLayer.hpp>
#include <core/net/NetworkManagerImpl.hpp>
#include <globed/core/Core.hpp>

using namespace geode::prelude;

namespace globed {

GlobalTriggersModule::GlobalTriggersModule() {}

void GlobalTriggersModule::onModuleInit()
{
    log::info("Global triggers module initialized");
    this->setAutoEnableMode(AutoEnableMode::Level);
}

void GlobalTriggersModule::queueCounterChange(const CounterChange &change)
{
    CounterChangeEvent ev{};
    ev.rawType = static_cast<uint8_t>(change.type);
    ev.itemId = change.itemId;

    log::debug("Queuing counter change: type={}, itemId={}, value={}", (int)change.type, change.itemId,
               change.value.asInt());

    uint64_t rawData =
        (static_cast<uint64_t>(change.type) << 56) | ((static_cast<uint64_t>(change.itemId) & 0x00ffffffull) << 32);

    if (change.type == CounterChangeType::Set || change.type == CounterChangeType::Add) {
        ev.rawValue = (uint32_t)change.value.asInt();
    } else {
        ev.rawValue = std::bit_cast<uint32_t>(change.value.asFloat());
    }

    NetworkManagerImpl::get().queueGameEvent(std::move(ev));
}

void GlobalTriggersModule::onPlayerJoin(GlobedGJBGL *gjbgl, int accountId)
{
    GTriggersGJBGL::get(gjbgl)->handlePlayerJoin(accountId);
}

void GlobalTriggersModule::onPlayerLeave(GlobedGJBGL *gjbgl, int accountId)
{
    GTriggersGJBGL::get(gjbgl)->handlePlayerLeave(accountId);
}

} // namespace globed
