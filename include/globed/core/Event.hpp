#pragma once
#include "data/Messages.hpp"
#include "net/MessageListener.hpp"
#include <span>

namespace globed {

enum class EventServer {
    Central = 1,
    Game = 2,
    Both = 3,
};

template <typename T>
concept EventDefinesServer = requires {
    { T::Type } -> std::same_as<EventServer>;
};

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

struct EventSendOptions {
    EventSendOptions() = default;

    EventServer server = EventServer::Central;
    std::vector<int> targetPlayers;
    bool reliable = false;
    bool urgent = false;
    char _reserved[88] = {};
};
static_assert(sizeof(EventSendOptions) == 128);

template <ValidEventType Derived>
struct ServerEvent {
    static EventServer server() {
        if constexpr (EventDefinesServer<Derived>) {
            return Derived::Type;
        } else {
            return EventServer::Both;
        }
    }

    static std::string_view id() {
        return Derived::Id;
    }

    /// Defined in soft-link/Wrappers.hpp
    static void _register();
    void send(this const Derived& self, const EventSendOptions& options = {});

    template <typename F>
    static geode::ListenerHandle listen(F callback, int priority = 0) {
        return MessageEvent<msg::LevelDataMessage>{false}.listen([cb = std::move(callback)](const msg::LevelDataMessage& data) {
            for (auto& ev : data.events) {
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

}
