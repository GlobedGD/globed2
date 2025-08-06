#include "GJBaseGameLayer.hpp"

#include <globed/util/Random.hpp>
#include <core/net/NetworkManagerImpl.hpp>
#include <modules/scripting/data/SpawnData.hpp>
#include <modules/scripting/data/SetItemData.hpp>

using namespace geode::prelude;

namespace globed {

SCBaseGameLayer* SCBaseGameLayer::get(GJBaseGameLayer* base) {
    if (!base) base = SCBaseGameLayer::get();

    return static_cast<SCBaseGameLayer*>(base);
}

void SCBaseGameLayer::postInit() {
    m_fields->m_listener = NetworkManagerImpl::get().listen<msg::LevelDataMessage>([this](const auto& msg) {
        auto& fields = *m_fields.self();

        for (auto& event : msg.events) {
            this->handleEvent(event);
        }

        return ListenerResult::Continue;
    });
}

void SCBaseGameLayer::handleEvent(const Event& event) {
    auto& fields = *m_fields.self();
    qn::ByteReader reader{event.data};

    switch (event.type) {
        case EVENT_SPAWN_GROUP: {
            auto res = globed::decodeSpawnData(reader);
            if (!res) {
                log::warn("Failed to decode spawn group data: {}", res.unwrapErr());
                return;
            }

            auto data = std::move(res).unwrap();
            double delay = data.delay.value_or(0.f);

            if (data.delayVariance > 0.f) {
                double min = std::max(0.0, delay - data.delayVariance);
                double max = delay + data.delayVariance;
                delay = rng()->random(min, max);
            }

            log::debug("Spawning group {} with delay {} (ordered: {})", data.groupId, delay, data.ordered);

            this->spawnGroup(
                data.groupId,
                data.ordered,
                delay,
                data.remaps,
                0,
                0
            );
        } break;

        case EVENT_SET_ITEM: {
            auto res = globed::decodeSetItemData(reader);
            if (!res) {
                log::warn("Failed to decode set item data: {}", res.unwrapErr());
                return;
            }

            auto data = std::move(res).unwrap();
            m_effectManager->updateCountForItem(data.itemId, data.value);
        } break;
    }

    // auto it = fields.m_customListeners.find(event.type);
    // if (it == fields.m_customListeners.end()) {
    //     return;
    // }

    // for (auto groupId : it->second) {
    //     // TODO: figure out the arguments to pass
    //     this->spawnGroup(groupId, true, 0.0, {}, 0, 0);
    // }
}

void SCBaseGameLayer::addEventListener(const ListenEventPayload& obj) {
    // m_fields->m_customListeners[obj.eventId].push_back(obj.groupId);
}

}
