#pragma once

#include "../core/data/Event.hpp"
#include "../core/data/FeaturedLevel.hpp"
#include "../core/data/UserRole.hpp"
#include "../core/net/ConnectionState.hpp"
#include "FunctionTable.hpp"

#ifdef GLOBED_API_EXT_FUNCTIONS
#include "../core/net/MessageListener.hpp"
#include <std23/move_only_function.h>
#endif

#define API_TABLE_FN(...) GEODE_INVOKE(GEODE_CONCAT(API_TABLE_FN_, GEODE_NUMBER_OF_ARGS(__VA_ARGS__)), __VA_ARGS__)

#define API_TABLE_FN_2(ret, name)                                                                                      \
    inline ::geode::Result<ret> name()                                                                                 \
    {                                                                                                                  \
        return this->invoke<ret>(#name);                                                                               \
    }
#define API_TABLE_FN_3(ret, name, p1)                                                                                  \
    inline ::geode::Result<ret> name(p1 a1)                                                                            \
    {                                                                                                                  \
        return this->invoke<ret>(#name, a1);                                                                           \
    }
#define API_TABLE_FN_4(ret, name, p1, p2)                                                                              \
    inline ::geode::Result<ret> name(p1 a1, p2 a2)                                                                     \
    {                                                                                                                  \
        return this->invoke<ret>(#name, a1, a2);                                                                       \
    }
#define API_TABLE_FN_5(ret, name, p1, p2, p3)                                                                          \
    inline ::geode::Result<ret> name(p1 a1, p2 a2, p3 a3)                                                              \
    {                                                                                                                  \
        return this->invoke<ret>(#name, a1, a2, a3);                                                                   \
    }
#define API_TABLE_FN_6(ret, name, p1, p2, p3, p4)                                                                      \
    inline ::geode::Result<ret> name(p1 a1, p2 a2, p3 a3, p4 a4)                                                       \
    {                                                                                                                  \
        return this->invoke<ret>(#name, a1, a2, a3, a4);                                                               \
    }
#define API_TABLE_FN_7(ret, name, p1, p2, p3, p4, p5)                                                                  \
    inline ::geode::Result<ret> name(p1 a1, p2 a2, p3 a3, p4 a4, p5 a5)                                                \
    {                                                                                                                  \
        return this->invoke<ret>(#name, a1, a2, a3, a4, a5);                                                           \
    }
#define API_TABLE_FN_8(ret, name, p1, p2, p3, p4, p5, p6)                                                              \
    inline ::geode::Result<ret> name(p1 a1, p2 a2, p3 a3, p4 a4, p5 a5, p6 a6)                                         \
    {                                                                                                                  \
        return this->invoke<ret>(#name, a1, a2, a3, a4, a5, a6);                                                       \
    }
#define API_TABLE_FN_9(ret, name, p1, p2, p3, p4, p5, p6, p7)                                                          \
    inline ::geode::Result<ret> name(p1 a1, p2 a2, p3 a3, p4 a4, p5 a5, p6 a6, p7 a7)                                  \
    {                                                                                                                  \
        return this->invoke<ret>(#name, a1, a2, a3, a4, a5, a6, a7);                                                   \
    }
#define API_TABLE_FN_10(ret, name, p1, p2, p3, p4, p5, p6, p7, p8)                                                     \
    inline ::geode::Result<ret> name(p1 a1, p2 a2, p3 a3, p4 a4, p5 a5, p6 a6, p7 a7, p8 a8)                           \
    {                                                                                                                  \
        return this->invoke<ret>(#name, a1, a2, a3, a4, a5, a6, a7, a8);                                               \
    }
#define API_TABLE_FN_11(ret, name, p1, p2, p3, p4, p5, p6, p7, p8, p9)                                                 \
    inline ::geode::Result<ret> name(p1 a1, p2 a2, p3 a3, p4 a4, p5 a5, p6 a6, p7 a7, p8 a8, p9 a9)                    \
    {                                                                                                                  \
        return this->invoke<ret>(#name, a1, a2, a3, a4, a5, a6, a7, a8, a9);                                           \
    }

namespace globed {

struct GlobedApiTable;

struct NetSubtable : public FunctionTableSubcat<GlobedApiTable> {
    API_TABLE_FN(void, disconnect);
    API_TABLE_FN(bool, isConnected);
    API_TABLE_FN(ConnectionState, getConnectionState);

    API_TABLE_FN(uint32_t, getCentralPingMs);
    API_TABLE_FN(uint32_t, getGamePingMs);
    API_TABLE_FN(uint32_t, getGameTickrate);

    API_TABLE_FN(std::vector<UserRole>, getAllRoles);
    API_TABLE_FN(std::vector<UserRole>, getUserRoles);
    API_TABLE_FN(std::vector<uint8_t>, getUserRoleIds);
    API_TABLE_FN(std::optional<UserRole>, getUserHighestRole);
    API_TABLE_FN(std::optional<UserRole>, findRole, uint8_t);
    API_TABLE_FN(std::optional<UserRole>, findRole, std::string_view);
    API_TABLE_FN(bool, isModerator);
    API_TABLE_FN(bool, isAuthorizedModerator);

    API_TABLE_FN(void, invalidateIcons);
    API_TABLE_FN(void, invalidateFriendList);

    API_TABLE_FN(std::optional<FeaturedLevelMeta>, getFeaturedLevel);
    API_TABLE_FN(void, queueGameEvent, OutEvent &&);

#ifdef GLOBED_API_EXT_FUNCTIONS
    API_TABLE_FN(void, addListener, const std::type_info &, void *, void *);
    API_TABLE_FN(void, removeListener, const std::type_info &, void *);

    template <typename T>
    [[nodiscard("listen returns a listener that must be kept alive to receive messages")]]
    MessageListener<T> listen(ListenerFn<T> callback)
    {
        auto listener = new MessageListenerImpl<T>(std::move(callback));
        this->addListener(
            typeid(T), listener, (void *)+[](void *ptr) { delete static_cast<MessageListenerImpl<T> *>(ptr); });
        return MessageListener<T>(listener);
    }

    template <typename T> MessageListenerImpl<T> *listenGlobal(ListenerFn<T> callback)
    {
        auto listener = new MessageListenerImpl<T>(std::move(callback));
        this->addListener(
            typeid(T), listener, (void *)+[](void *ptr) { delete static_cast<MessageListenerImpl<T> *>(ptr); });
        return listener;
    }
#endif
};

struct GlobedApiTable : public FunctionTable {
    NetSubtable net{this, "net"};
};

} // namespace globed

#undef VA_NARGS_IMPL
#undef VA_NARGS
#undef API_TABLE_FN_0
#undef API_TABLE_FN_1
#undef API_TABLE_FN_2
#undef API_TABLE_FN_3
#undef API_TABLE_FN_4
#undef API_TABLE_FN_5
#undef API_TABLE_FN_6
#undef API_TABLE_FN_7
#undef API_TABLE_FN_8
#undef API_TABLE_FN_9
#undef API_TABLE_FN_10
#undef API_TABLE_FN
