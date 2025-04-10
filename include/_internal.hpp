#pragma once
#ifndef GLOBED_INTERNAL_HPP
#define GLOBED_INTERNAL_HPP

#include <Geode/loader/Event.hpp>
#include <Geode/Result.hpp>

namespace globed {

template <typename T = void, typename E = std::string>
using Result = geode::Result<T, E>;
using geode::Ok;
using geode::Err;

namespace _internal {

// Each event has a unique code assigned to it.
enum class Type {
    // Player
    IsGlobedPlayer = 1100,
    ExplodeRandomPlayer = 1101,
    AccountIdForPlayer = 1102,
    PlayersOnLevel = 1103,
    PlayersOnline = 1104,
    AllPlayerIds = 1105,
    PlayerObjectsForId = 1106,
    // Networking
    IsConnected = 1200,
    ServerTps = 1201,
    Ping = 1202,
    IsStandalone = 1203,
    IsReconnecting = 1204,
    // Admin
    IsMod = 1301,
    IsAuthMod = 1302,
    OpenModPanel = 1303,
    // Callbacks
    CbPlayerJoin = 1400,
    CbPlayerLeave = 1401,
    // Settings
    ActiveSlotPath = 1500,
    ActiveSlotContainer = 1501,
    ReloadSaveContainer = 1502,
    LaunchFlag = 1503,
};

class BaseEvent : public geode::Event {
public:
    explicit BaseEvent(Type type) : type(type) {}

    Type type;
};

template <typename Ret, typename... Args>
class RequestEvent : public BaseEvent {
public:
    RequestEvent(Type type, const Args&... args) :
        BaseEvent(type),
        args(args...),
        // We preset this to an error value so that we can detect if the dispatcher did not respond at all.
        result(Err("_placeholder")) {}

    std::tuple<Args...> args;
    Result<Ret> result;

    // This method should only ever be called by the event dispatcher, aka Globed itself.
    template <class A = Ret, class = std::enable_if_t<!std::is_void_v<A>>>
    void respond(A&& value) {
        result = Ok(std::move(value));
    }

    // This method should only ever be called by the event dispatcher, aka Globed itself.
    template <class A = Ret, class = std::enable_if_t<!std::is_void_v<A>>>
    void respond(const A& value) {
        result = Ok(std::move(value));
    }

    // This method should only ever be called by the event dispatcher, aka Globed itself.
    void respond() requires std::is_void_v<Ret> {
        result = Ok();
    }

    // This method should only ever be called by the event dispatcher, aka Globed itself.
    void respondError(const std::string& error) {
        result = Err(error);
    }
};

template <typename Ret, typename... Args>
inline Result<Ret> request(Type type, const Args&... args) {
    RequestEvent<Ret, Args...> event(type, args...);
    event.post();

    if (event.result.isErr()) {
        auto& err = event.result.unwrapErr();
        if (err == "_placeholder") {
            // This means the dispatcher did not respond to the event. Likely Globed is not loaded.
            return Err("No response received - is Globed currently loaded?");
        } else {
            return std::move(event.result);
        }
    }

    return std::move(event.result);
}

template <typename... Args>
inline Result<void> requestVoid(Type type, const Args&... args) {
    return request<void, Args...>(type, args...);
}

} // namespace _internal
} // namespace globed

#endif // GLOBED_INTERNAL_HPP
