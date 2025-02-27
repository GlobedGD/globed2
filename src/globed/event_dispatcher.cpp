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

template <Type WantedType, typename Ret = void, typename... Args>
void listen(auto function) {
    using EventType = RequestEvent<Ret, Args...>;
    using EventResult = Result<Ret>;

    std::function<EventResult (const Args&...)> listener = function;

    new geode::EventListener<EventFilter<EventType>>([listener = std::move(listener)](EventType* event) {
        if (event->type == WantedType) {
            event->result = listener(std::get<Args>(event->args)...);
            return ListenerResult::Stop;
        }

        return ListenerResult::Propagate;
    });
}

$on_mod(Loaded) {
    using enum ListenerResult;

    // Player

    listen<Type::IsGlobedPlayer, bool, PlayerObject*>([](PlayerObject* node) {
        auto cvp = typeinfo_cast<ComplexVisualPlayer*>(node->getParent());

        return Ok(cvp != nullptr && cvp->getPlayerObject() == node);
    });

    listen<Type::ExplodeRandomPlayer>([]() -> Result<void> {
        auto gjbgl = GlobedGJBGL::get();

        log::debug("API call ExplodeRandomPlayer!");

        if (gjbgl && gjbgl->established()) {
            gjbgl->explodeRandomPlayer();
            return Ok();
        } else {
            return Err(NOT_CONNECTED_MSG);
        }
    });

    listen<Type::AccountIdForPlayer, int, PlayerObject*>([](PlayerObject* node) -> Result<int> {
        auto cvp = typeinfo_cast<ComplexVisualPlayer*>(node->getParent());
        if (cvp) {
            auto rp = cvp->getRemotePlayer();
            if (rp) {
                return Ok(rp->getAccountData().accountId);
            }
        }

        return Ok(-1);
    });

    listen<Type::PlayersOnLevel, size_t>([]() -> Result<size_t> {
        auto gjbgl = GlobedGJBGL::get();
        if (gjbgl && gjbgl->established()) {
            return Ok(gjbgl->getFields().players.size());
        }

        return Err(NOT_CONNECTED_MSG);
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

    // Settings

    listen<Type::ActiveSlotPath, std::filesystem::path>([] {
        auto& gs = GlobedSettings::get();
        return Ok(gs.pathForSlot(gs.getSelectedSlot()));
    });

    listen<Type::ActiveSlotContainer, matjson::Value>([] {
        auto& gs = GlobedSettings::get();
        return Ok(gs.getSettingContainer());
    });

    listen<Type::ReloadSaveContainer, void, matjson::Value>([](const matjson::Value& container) {
        auto& gs = GlobedSettings::get();
        gs.reloadFromContainer(container);
        return Ok();
    });

    listen<Type::LaunchFlag, bool, std::string_view>([](std::string_view flag) {
        std::string flagStr(flag);
        if (!flagStr.starts_with("globed-")) {
            flagStr.insert(0, "globed-");
        }

        return Ok(Loader::get()->getLaunchFlag(flagStr));
    });
}

