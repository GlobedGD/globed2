#pragma once

#include <globed/util/singleton.hpp>
#include <globed/core/net/MessageListener.hpp>
#include <globed/core/data/UserRole.hpp>
#include <globed/core/data/Event.hpp>
#include <globed/core/data/ModPermissions.hpp>
#include <globed/core/data/SpecialUserData.hpp>
#include <globed/core/data/FeaturedLevel.hpp>
#include "ConnectionState.hpp"

#include <asp/time/Duration.hpp>
#include <memory>

namespace globed {

class NetworkManagerImpl;


class GLOBED_DLL NetworkManager : public SingletonBase<NetworkManager> {
public:
    // Connect to the central server at the given URL.
    // See qunet's documentation for the URL format.
    geode::Result<> connectCentral(std::string_view url);
    geode::Result<> disconnectCentral();
    geode::Result<> cancelConnection();

    ConnectionState getConnectionState();

    /// Returns whether the client is connected and authenticated with the central server
    bool isConnected() const;

    /// Returns the average latency to the game server, or 0 if not connected
    asp::time::Duration getGamePing();
    /// Returns the average latency to the central server, or 0 if not connected
    asp::time::Duration getCentralPing();

    /// Returns the tickrate to the connected game server, or 0 if not connected
    uint32_t getGameTickrate();

    std::vector<UserRole> getAllRoles();
    std::vector<UserRole> getUserRoles();
    std::vector<uint8_t> getUserRoleIds();
    std::optional<UserRole> getUserHighestRole();
    std::optional<UserRole> findRole(uint8_t roleId);
    std::optional<UserRole> findRole(std::string_view roleId);
    bool isModerator();
    bool isAuthorizedModerator();
    ModPermissions getModPermissions();
    std::optional<SpecialUserData> getOwnSpecialData();

    /// Force the client to resend user icons to the connected server. Does nothing if not connected.
    void invalidateIcons();
    /// Force the client to resend the friend list to the connected server. Does nothing if not connected.
    void invalidateFriendList();

    /// Get the ID of the current featured level on this server
    std::optional<FeaturedLevelMeta> getFeaturedLevel();
    bool hasViewedFeaturedLevel();
    void setViewedFeaturedLevel();

    void queueGameEvent(OutEvent&& event);

    // Listeners

    template <typename T>
    [[nodiscard("listen returns a listener that must be kept alive to receive messages")]]
    MessageListener<T> listen(ListenerFn<T> callback) {
        auto listener = new MessageListenerImpl<T>(std::move(callback));
        this->addListener(typeid(T), listener, (void*) +[](void* ptr) {
            delete static_cast<MessageListenerImpl<T>*>(ptr);
        });
        return MessageListener<T>(listener);
    }

    template <typename T>
    MessageListenerImpl<T>* listenGlobal(ListenerFn<T> callback) {
        auto listener = new MessageListenerImpl<T>(std::move(callback));
        this->addListener(typeid(T), listener, (void*) +[](void* ptr) {
            delete static_cast<MessageListenerImpl<T>*>(ptr);
        });
        return listener;
    }

    void addListener(const std::type_info& ty, void* listener, void* dtor);
    void removeListener(const std::type_info& ty, void* listener);

private:
    friend class SingletonBase;
    friend class NetworkManagerImpl;

    std::unique_ptr<NetworkManagerImpl> m_impl;

    NetworkManager();
    ~NetworkManager();
};

inline void _destroyListener(const std::type_info& ty, void* ptr) {
    NetworkManager::get().removeListener(ty, ptr);
}

}
