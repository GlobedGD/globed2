#include "GJBaseGameLayer.hpp"

#include <core/net/NetworkManagerImpl.hpp>

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

    auto it = fields.m_customListeners.find(event.type);
    if (it == fields.m_customListeners.end()) {
        return;
    }

    for (auto groupId : it->second) {
        // TODO: figure out the arguments to pass
        this->spawnGroup(groupId, true, 0.0, {}, 0, 0);
    }
}

void SCBaseGameLayer::addEventListener(const ListenEventPayload& obj) {
    m_fields->m_customListeners[obj.eventId].push_back(obj.groupId);
}

}
