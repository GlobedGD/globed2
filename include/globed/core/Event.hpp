#pragma once
#include "data/Messages.hpp"
#include "net/MessageListener.hpp"
#include "../soft-link/API.hpp"
#include <span>

namespace globed {

template <typename T>
concept ValidEventType = requires(const T t, std::span<const uint8_t> data) {
    { T::Id } -> std::convertible_to<std::string_view>;

};

template <typename T>
concept EncodableEvent = ValidEventType<T> && requires(const T t) {
    { t.encode() } -> std::convertible_to<std::vector<uint8_t>>;
};

template <typename T>
concept DecodableEvent = ValidEventType<T> && requires(std::span<const uint8_t> data) {
    { T::decode(data) } -> std::convertible_to<geode::Result<T>>;
};

template <typename Derived>
struct ServerEventAutoInit {
    ServerEventAutoInit() {
        Derived::_register();
    }
};

struct SkipRemainingEvents {};

template <typename F, typename E>
bool invokeEventCallback(F&& cb, const E& e, const EventOptions& options);

template <typename Derived, EventServer Server = EventServer::Both>
struct ServerEvent {
    ServerEvent() = default;

    static EventServer server() {
        return Server;
    }

    static std::string_view id() {
        return Derived::Id;
    }

    static void _register();

    void send(this const Derived& self, EventOptions options) requires EncodableEvent<Derived> {
        // if a server was not specified, set it to the supported one, or Central if both are enabled
        if (options.server == EventServer::Auto) {
            options.server = Server == EventServer::Both ? EventServer::Central : Server;
        }

        globed::api::net::sendEvent(self.id(), self.encode(), options);
    }

    /// Sends this event to the specified server.
    /// All the send options are default, and the event is sent to everybody in the room/session.
    void send(this const Derived& self, EventServer server) requires EncodableEvent<Derived> {
        EventOptions opts{};
        opts.server = server;
        return self.send(std::move(opts));
    }

    /// Sends this event to the default server, targetting a specific player.
    void send(this const Derived& self, int playerId) requires EncodableEvent<Derived> {
        EventOptions opts{};
        opts.targetPlayers.push_back(playerId);
        return self.send(std::move(opts));
    }

    /// Sends this event to the default server, which will be Central if both are enabled.
    /// All the send options are default, and the event is sent to everybody in the room/session.
    void send(this const Derived& self) requires EncodableEvent<Derived> {
        self.send(EventOptions{});
    }

    template <typename F>
    static geode::ListenerHandle listen(F callback, int priority = 0) requires DecodableEvent<Derived> {
        return MessageEvent<msg::EventsMessage>{false}.listen([cb = std::move(callback)](const msg::EventsMessage& data) {
            for (auto& ev : data.events) {
                if (ev.name != id()) continue;

                auto dec = Derived::decode(ev.data);
                if (dec.isOk()) {
                    auto skip = invokeEventCallback(cb, std::move(dec).unwrap(), ev.options);
                    if (skip) {
                        break;
                    }
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
    static_assert(ValidEventType<Derived>, "All events must define an ID");
    globed::api::waitForGlobed([] {
        globed::api::net::registerEvent(Derived::id(), Derived::server());
    });
}

template <typename F, typename E>
bool invokeEventCallback(F&& cb, const E& e, const EventOptions& options) {
    auto invoke = [&] {
        if constexpr (std::is_invocable_v<F, const E&, const EventOptions&>) {
            return std::invoke(cb, e, options);
        } else if constexpr (std::is_invocable_v<F, const E&>) {
            return std::invoke(cb, e);
        } else {
            return std::invoke(cb);
        }
    };

    using Ret = decltype(invoke());
    invoke();

    return std::is_same_v<Ret, SkipRemainingEvents>;
}

}
