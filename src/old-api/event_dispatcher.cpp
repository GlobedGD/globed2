#include "include/globed.hpp"

#include <core/hooks/GJBaseGameLayer.hpp>
#include <core/net/NetworkManagerImpl.hpp>
#include <globed/core/ServerManager.hpp>
#include <globed/core/SettingsManager.hpp>
#include <globed/core/game/RemotePlayer.hpp>
#include <globed/core/game/VisualPlayer.hpp>

using namespace geode::prelude;
using namespace globed::_internal;

constexpr const char* NOT_CONNECTED_MSG = "Not connected to a server";
constexpr const char* NOT_IN_LEVEL_MSG = "Not currently in a level";

// Helper struct and specializations for obtaining function return type and arguments
template <typename F>
struct function_traits;

// Specialization for function objects
template <typename Ret, typename... Args>
struct function_traits<Ret(Args...)> {
    using return_type = Ret;
    using arg_types = std::tuple<Args...>;
};

// Specialization for function pointers
template <typename Ret, typename... Args>
struct function_traits<Ret(*)(Args...)> {
    using return_type = Ret;
    using arg_types = std::tuple<Args...>;
};

// Specialization for callable objects like lambdas
template <typename F>
struct function_traits : function_traits<decltype(&F::operator())> {};

template <typename C, typename Ret, typename... Args>
struct function_traits<Ret(C::*)(Args...) const> {
    using return_type = Ret;
    using arg_types = std::tuple<Args...>;
};

template <template <typename, typename...> typename Event, typename Ret, typename ArgsTuple>
struct make_event;

template <template <typename, typename...> typename Event, typename Ret, typename... Args>
struct make_event<Event, Ret, std::tuple<Args...>> {
    using Type = Event<Ret, Args...>;
};

template <typename Ret, typename ArgsTuple>
struct make_event_handler_fn;

template <typename Ret, typename... Args>
struct make_event_handler_fn<Ret, std::tuple<Args...>> {
    using Type = Result<Ret> (const Args&...);
};

template <typename F>
struct convert_event_fn {
    using type = F();
};

template <typename Ret, typename... Args>
struct convert_event_fn<Ret(Args...)> {
    using type = Ret(Args...);
};

template <typename Listener, typename ArgsTuple>
auto invoke_listener(Listener&& listener, ArgsTuple&& args) {
    return std::apply(listener, std::forward<ArgsTuple>(args));
}

// Specialization for zero arguments
template <typename Listener>
auto invoke_listener(Listener&& listener, std::tuple<>&&) {
    return listener();
}

// TODO: all of this is super devious, test it properly
template <Type WantedType, typename EventFn_ = void(), typename CallbackFn = Result<>()>
void listen(CallbackFn&& function) {
    // EventFn is the signature of the event. Has to be defined exactly and correctly, for example `bool(PlayerObject*)`
    // NOTE: EventFn can also be just the return type, then it is assumed there are no arguments.
    // CallbackFn is the signature of the callback function. It can be a lambda, a function pointer, or a function object.
    // Its arguments should be the same as EventFn, and the return type is Result<T> where T is the return type of the event.

    // if EventFn_ is a function, keep as is, otherwise, convert it to `EventFn_()`
    using EventFn = convert_event_fn<EventFn_>::type;

    // EventRet will be the actual return type the user *cares* about, so not wrapped in Result<>
    using EventRet = typename function_traits<EventFn>::return_type;

    // EventArgsTuple will be an std::tuple<Args...> of arguments the event takes
    using EventArgsTuple = typename function_traits<EventFn>::arg_types;

    // EventType will be RequestEvent<EventRet, Args...>, we use make_event to be able to unpack the tuple of arguments
    using EventType = typename make_event<RequestEvent, EventRet, EventArgsTuple>::Type;

    new geode::EventListener<EventFilter<EventType>>([listener = std::forward<CallbackFn>(function)](EventType* event) {
        if (event->type == WantedType) {
            event->result = invoke_listener(listener, event->args);
            return ListenerResult::Stop;
        }

        return ListenerResult::Propagate;
    });
}

$on_mod(Loaded) {
    using enum ListenerResult;

    // Player

    listen<Type::IsGlobedPlayer, bool(PlayerObject*)>([](PlayerObject* node) {
        return Ok(typeinfo_cast<globed::VisualPlayer*>(node));
    });

    listen<Type::ExplodeRandomPlayer>([]() -> Result<void> {
        return Err("Not implemented; obsolete since globed v2");
    });

    listen<Type::AccountIdForPlayer, int(PlayerObject*)>([](PlayerObject* node) -> Result<int> {
        if (auto cvp = typeinfo_cast<globed::VisualPlayer*>(node)) {
            return Ok(cvp->displayData().accountId);
        }

        return Err("Invalid player");
    });

    listen<Type::PlayersOnLevel, size_t>([]() -> Result<size_t> {
        auto gjbgl = globed::GlobedGJBGL::get();

        if (!gjbgl) {
            return Err(NOT_IN_LEVEL_MSG);
        }

        if (!gjbgl->active()) {
            return Err(NOT_CONNECTED_MSG);
        }

        return Ok(gjbgl->m_fields->m_players.size());
    });

    listen<Type::PlayersOnline, size_t>([]() -> Result<size_t> {
        return Ok(0);
    });

    listen<Type::AllPlayerIds, std::vector<int>>([]() -> Result<std::vector<int>> {
        auto gjbgl = globed::GlobedGJBGL::get();

        if (!gjbgl) {
            return Err(NOT_IN_LEVEL_MSG);
        }

        if (!gjbgl->active()) {
            return Err(NOT_CONNECTED_MSG);
        }

        std::vector<int> values;

        auto& players = gjbgl->m_fields->m_players;
        for (const auto& [key, val] : players) {
            values.push_back(key);
        }

        return Ok(std::move(values));
    });

    listen<Type::PlayerObjectsForId, std::pair<PlayerObject*, PlayerObject*>(int)>([](int id) -> Result<std::pair<PlayerObject*, PlayerObject*>> {
        auto gjbgl = globed::GlobedGJBGL::get();

        if (!gjbgl) {
            return Err(NOT_IN_LEVEL_MSG);
        }

        if (!gjbgl->active()) {
            return Err(NOT_CONNECTED_MSG);
        }

        auto& players = gjbgl->m_fields->m_players;
        auto it = players.find(id);

        if (it == players.end()) {
            return Err("Player not found");
        }

        auto& pl = it->second;
        return Ok(std::make_pair(
            pl->player1(),
            pl->player2()
        ));
    });

    // Networking

    listen<Type::IsConnected, bool>([] {
        return Ok(globed::NetworkManagerImpl::get().isConnected());
    });

    listen<Type::ServerTps, uint32_t>([] {
        return Ok(globed::NetworkManagerImpl::get().getGameTickrate());
    });

    listen<Type::Ping, uint32_t>([] {
        auto& nm = globed::NetworkManagerImpl::get();
        return Ok(nm.getGamePing().millis());
    });

    listen<Type::IsStandalone, bool>([] {
        return Err("Not implemented; obsolete since globed v2");
    });

    listen<Type::IsReconnecting, bool>([] {
        return Ok(globed::NetworkManagerImpl::get().getConnState(false) == qn::ConnectionState::Reconnecting);
    });

    // Admin

    listen<Type::IsMod, bool>([] {
        return Ok(globed::NetworkManagerImpl::get().isModerator());
    });

    listen<Type::IsAuthMod, bool>([] {
        return Ok(globed::NetworkManagerImpl::get().isAuthorizedModerator());
    });

    listen<Type::OpenModPanel>([] {
        globed::openModPanel();
        return Ok();
    });

    // Callbacks

    using namespace globed::callbacks;

    listen<Type::CbPlayerJoin, void(PlayerJoinFn)>([](PlayerJoinFn fn) -> Result<void> {
        return Err("Not implemented; obsolete since globed v2");
    });

    listen<Type::CbPlayerLeave, void(PlayerLeaveFn)>([](PlayerLeaveFn fn) -> Result<void> {
        return Err("Not implemented; obsolete since globed v2");
    });

    // Settings

    listen<Type::ActiveSlotPath, std::filesystem::path>([] {
        return Err("Not implemented; obsolete since globed v2");
    });

    listen<Type::ActiveSlotContainer, matjson::Value>([] {
        return Err("Not implemented; obsolete since globed v2");
    });

    listen<Type::ReloadSaveContainer, void(matjson::Value)>([](const matjson::Value& container) {
        return Err("Not implemented; obsolete since globed v2");
    });

    listen<Type::LaunchFlag, bool(std::string_view)>([](std::string_view flag) {
        return Err("Not implemented; obsolete since globed v2");
    });
}

