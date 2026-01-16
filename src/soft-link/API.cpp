#define GEODE_DEFINE_EVENT_EXPORTS

#include <asp/format.hpp>
#include <core/net/NetworkManagerImpl.hpp>
#include <globed/core/net/NetworkManager.hpp>
#include <globed/soft-link/API.hpp>

using namespace geode::prelude;

namespace globed {

static GlobedApiTable s_functionTable;

Result<FunctionTable *> getFunctionTable()
{
    return Ok(&s_functionTable);
}

struct Assigner {
    std::string_view cat, name;

    template <typename T> Assigner &operator=(T &&func)
    {
        char buf[128];
        auto fullName = asp::local_format(buf, "{}.{}", cat, name);

#ifdef GLOBED_DEBUG
        log::debug("Adding soft api function: '{}'", fullName);
#endif

        s_functionTable.insert(fullName, std::forward<T>(func));
        return *this;
    }
};

static auto assigner(std::string_view cat, std::string_view name)
{
    return Assigner{cat, name};
}

#define TABLE_FN(ret, name, ...) assigner(category, #name) = +[](__VA_ARGS__)->Result<ret>

static void addNetFunctions()
{
    auto category = "net";

    TABLE_FN(void, disconnect)
    {
        NetworkManagerImpl::get().disconnectCentral();
        return Ok();
    };

    TABLE_FN(bool, isConnected)
    {
        return Ok(NetworkManagerImpl::get().isConnected());
    };

    TABLE_FN(ConnectionState, getConnectionState)
    {
        return Ok(NetworkManager::get().getConnectionState());
    };

    TABLE_FN(uint32_t, getCentralPingMs)
    {
        return Ok(NetworkManagerImpl::get().getCentralPing().millis());
    };

    TABLE_FN(uint32_t, getGamePingMs)
    {
        return Ok(NetworkManagerImpl::get().getGamePing().millis());
    };

    TABLE_FN(uint32_t, getGameTickrate)
    {
        return Ok(NetworkManagerImpl::get().getGameTickrate());
    };

    TABLE_FN(std::vector<UserRole>, getAllRoles)
    {
        return Ok(NetworkManagerImpl::get().getAllRoles());
    };

    TABLE_FN(std::vector<UserRole>, getUserRoles)
    {
        return Ok(NetworkManagerImpl::get().getUserRoles());
    };

    TABLE_FN(std::vector<uint8_t>, getUserRoleIds)
    {
        return Ok(NetworkManagerImpl::get().getUserRoleIds());
    };

    TABLE_FN(std::optional<UserRole>, getUserHighestRole)
    {
        return Ok(NetworkManagerImpl::get().getUserHighestRole());
    };

    TABLE_FN(std::optional<UserRole>, findRole, uint8_t id)
    {
        return Ok(NetworkManagerImpl::get().findRole(id));
    };

    TABLE_FN(std::optional<UserRole>, findRole, std::string_view id)
    {
        return Ok(NetworkManagerImpl::get().findRole(id));
    };

    TABLE_FN(bool, isModerator)
    {
        return Ok(NetworkManagerImpl::get().isModerator());
    };

    TABLE_FN(bool, isAuthorizedModerator)
    {
        return Ok(NetworkManagerImpl::get().isAuthorizedModerator());
    };

    TABLE_FN(void, invalidateIcons)
    {
        NetworkManagerImpl::get().invalidateIcons();
        return Ok();
    };

    TABLE_FN(void, invalidateFriendList)
    {
        NetworkManagerImpl::get().invalidateFriendList();
        return Ok();
    };

    TABLE_FN(std::optional<FeaturedLevelMeta>, getFeaturedLevel)
    {
        return Ok(NetworkManagerImpl::get().getFeaturedLevel());
    };

    TABLE_FN(void, queueGameEvent, OutEvent &&event)
    {
        NetworkManagerImpl::get().queueGameEvent(std::move(event));
        return Ok();
    };

    TABLE_FN(void, addListener, const std::type_info &type, void *listener, void *deleter)
    {
        NetworkManagerImpl::get().addListener(type, listener, deleter);
        return Ok();
    };

    TABLE_FN(void, removeListener, const std::type_info &type, void *listener)
    {
        NetworkManagerImpl::get().removeListener(type, listener);
        return Ok();
    };
}

$execute
{
    addNetFunctions();

    // TODO more stuff

    s_functionTable.finalize();
}

} // namespace globed
