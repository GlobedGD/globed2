#include <globed.hpp>

#include <hooks/gjbasegamelayer.hpp>
#include <managers/account.hpp>
#include <managers/admin.hpp>
#include <managers/game_server.hpp>
#include <managers/settings.hpp>
#include <net/manager.hpp>
#include <ui/game/player/complex_visual_player.hpp>
#include <ui/game/player/remote_player.hpp>

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

    // EventHandlerFunction will be Result<EventRet> (const Args&...)
    // We will convert our CallbackFn to this type, and this is what callbacks should be like
    using EventHandlerFunction = typename make_event_handler_fn<EventRet, EventArgsTuple>::Type;

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
        auto cvp = typeinfo_cast<ComplexVisualPlayer*>(node->getParent());

        return Ok(cvp != nullptr && cvp->getPlayerObject() == node);
    });

    listen<Type::ExplodeRandomPlayer>([]() -> Result<void> {
        auto gjbgl = GlobedGJBGL::get();

        if (!gjbgl) {
            return Err(NOT_IN_LEVEL_MSG);
        }

        if (!gjbgl->established()) {
            return Err(NOT_CONNECTED_MSG);
        }

        gjbgl->explodeRandomPlayer();
        return Ok();
    });

    listen<Type::AccountIdForPlayer, int(PlayerObject*)>([](PlayerObject* node) -> Result<int> {
        auto cvp = typeinfo_cast<ComplexVisualPlayer*>(node->getParent());
        if (cvp) {
            auto rp = cvp->getRemotePlayer();
            if (rp) {
                return Ok(rp->getAccountData().accountId);
            }
        }

        return Err("Invalid player");
    });

    listen<Type::PlayersOnLevel, size_t>([]() -> Result<size_t> {
        auto gjbgl = GlobedGJBGL::get();

        if (!gjbgl) {
            return Err(NOT_IN_LEVEL_MSG);
        }

        if (!gjbgl->established()) {
            return Err(NOT_CONNECTED_MSG);
        }

        return Ok(gjbgl->getFields().players.size());
    });

    listen<Type::PlayersOnline, size_t>([]() -> Result<size_t> {
        auto& gsm = GameServerManager::get();
        auto activeServer = gsm.getActiveServer();

        if (!activeServer) {
            return Err(NOT_CONNECTED_MSG);
        }

        auto& server = activeServer.value();
        return Ok(server.playerCount);
    });

    listen<Type::AllPlayerIds, std::vector<int>>([]() -> Result<std::vector<int>> {
        auto gjbgl = GlobedGJBGL::get();

        if (!gjbgl) {
            return Err(NOT_IN_LEVEL_MSG);
        }

        if (!gjbgl->established()) {
            return Err(NOT_CONNECTED_MSG);
        }

        std::vector<int> values;

        auto& players = gjbgl->getFields().players;
        for (const auto& [key, val] : players) {
            values.push_back(key);
        }

        return Ok(std::move(values));
    });

    listen<Type::PlayerObjectsForId, std::pair<PlayerObject*, PlayerObject*>(int)>([](int id) -> Result<std::pair<PlayerObject*, PlayerObject*>> {
        auto gjbgl = GlobedGJBGL::get();

        if (!gjbgl) {
            return Err(NOT_IN_LEVEL_MSG);
        }

        if (!gjbgl->established()) {
            return Err(NOT_CONNECTED_MSG);
        }

        auto& players = gjbgl->getFields().players;
        auto it = players.find(id);

        if (it == players.end()) {
            return Err("Player not found");
        }

        auto pl = it->second;
        return Ok(std::make_pair(
            pl->player1->getPlayerObject(),
            pl->player2->getPlayerObject()
        ));
    });

    // Networking

    listen<Type::IsConnected, bool>([] {
        return Ok(NetworkManager::get().established());
    });

    listen<Type::ServerTps, uint32_t>([] {
        return Ok(NetworkManager::get().getServerTps());
    });

    listen<Type::Ping, uint32_t>([] {
        auto& gsm = GameServerManager::get();
        if (gsm.getActiveId().empty()) {
            return Ok(-1);
        }

        return Ok(gsm.getActivePing());
    });

    listen<Type::IsStandalone, bool>([] {
        return Ok(NetworkManager::get().standalone());
    });

    listen<Type::IsReconnecting, bool>([] {
        return Ok(NetworkManager::get().reconnecting());
    });

    // Admin

    listen<Type::IsMod, bool>([] {
        return Ok(AdminManager::get().getRole().canModerate());
    });

    listen<Type::IsAuthMod, bool>([] {
        return Ok(AdminManager::get().canModerate());
    });

    listen<Type::OpenModPanel>([] {
        AdminManager::get().openModPanel();
        return Ok();
    });

    // Callbacks

    using namespace globed::callbacks;

    listen<Type::CbPlayerJoin, void(PlayerJoinFn)>([](PlayerJoinFn fn) -> Result<void> {
        auto gjbgl = GlobedGJBGL::get();
        if (!gjbgl) {
            return Err(NOT_IN_LEVEL_MSG);
        }

        auto& fields = gjbgl->getFields();
        if (!fields.globedReady) {
            return Err(NOT_CONNECTED_MSG);
        }

        gjbgl->addPlayerJoinCallback(std::move(fn));

        return Ok();
    });

    listen<Type::CbPlayerLeave, void(PlayerLeaveFn)>([](PlayerLeaveFn fn) -> Result<void> {
        auto gjbgl = GlobedGJBGL::get();
        if (!gjbgl) {
            return Err(NOT_IN_LEVEL_MSG);
        }

        auto& fields = gjbgl->getFields();
        if (!fields.globedReady) {
            return Err(NOT_CONNECTED_MSG);
        }

        gjbgl->addPlayerLeaveCallback(std::move(fn));

        return Ok();
    });

    // Settings

    listen<Type::ActiveSlotPath, std::filesystem::path>([] {
        auto& gs = GlobedSettings::get();
        return Ok(gs.pathForSlot(gs.getSelectedSlot()));
    });

    listen<Type::ActiveSlotContainer, matjson::Value>([] {
        auto& gs = GlobedSettings::get();
        return Ok(gs.getSettingContainer());
    });

    listen<Type::ReloadSaveContainer, void(matjson::Value)>([](const matjson::Value& container) {
        auto& gs = GlobedSettings::get();
        gs.reloadFromContainer(container);
        return Ok();
    });

    listen<Type::LaunchFlag, bool(std::string_view)>([](std::string_view flag) {
        std::string flagStr(flag);
        if (!flagStr.starts_with("globed-")) {
            flagStr.insert(0, "globed-");
        }

        return Ok(Loader::get()->getLaunchFlag(flagStr));
    });
}

