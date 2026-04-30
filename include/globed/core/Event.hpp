#pragma once
#include "data/Messages.hpp"
#include "net/MessageListener.hpp"
#include "../soft-link/API.hpp"
#include <span>

namespace globed {

template <typename T>
concept ValidEventType = requires(const T t, std::span<const uint8_t> data) {
    { T::Id } -> std::convertible_to<std::string_view>;

    { t.encode() } -> std::convertible_to<std::vector<uint8_t>>;
    { T::decode(data) } -> std::convertible_to<geode::Result<T>>;
};

template <typename Derived>
struct ServerEventAutoInit {
    ServerEventAutoInit() {
        Derived::_register();
    }
};

template <typename Derived, EventServer Server = EventServer::Both>
struct ServerEvent {
    static EventServer server() {
        return Server;
    }

    static std::string_view id() {
        return Derived::Id;
    }

    static void _register();

    void send(this const Derived& self, const EventOptions& options) {
        globed::api::net::sendEvent(self.id(), self.encode(), options);
    }

    void send(this const Derived& self, EventServer server) {
        EventOptions opts{};
        opts.server = server;
        return self.send(opts);
    }

    void send(this const Derived& self) {
        EventOptions opts{};
        auto targetServer = self.server();

        if (targetServer == EventServer::Both) {
            // default to central server if enabled on both
            targetServer = EventServer::Central;
        }

        self.send(opts);
    }

    template <typename F>
    static geode::ListenerHandle listen(F callback, int priority = 0) {
        return MessageEvent<msg::EventsMessage>{false}.listen([cb = std::move(callback)](const msg::EventsMessage& data) {
            for (auto& ev : data.events) {
                if (ev.name != id()) continue;

                auto dec = Derived::decode(ev.data);
                if (dec.isOk()) {
                    cb(std::move(dec).unwrap());
                } else {
                    geode::log::warn("failed to decode event {}: {}", id(), dec.unwrapErr());
                }
            }
        }, priority);
    }

private:
    // automatically register when the mod is loaded
    static inline ServerEventAutoInit<Derived> s_autoInit;
    static inline auto s_autoInitRef = &ServerEvent::s_autoInit;
};

template <typename Derived, EventServer Server>
void ServerEvent<Derived, Server>::_register() {
    static_assert(ValidEventType<Derived>);
    globed::api::waitForGlobed([] {
        globed::api::net::registerEvent(Derived::id(), Derived::server());
    });
}

}
