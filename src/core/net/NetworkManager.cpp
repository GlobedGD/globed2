#include <globed/core/net/NetworkManager.hpp>

#include "NetworkManagerImpl.hpp"

using namespace geode::prelude;

namespace globed {

NetworkManager::NetworkManager() : m_impl(std::make_unique<NetworkManagerImpl>()) {}

NetworkManager::~NetworkManager() {
    log::info("goodbye from networkmanager!");

    // leak because cleanup causes more trouble than it's worth
    std::ignore = m_impl.release();
}

Result<> NetworkManager::connectCentral(std::string_view url) {
    return m_impl->connectCentral(url);
}

ConnectionState NetworkManager::getConnectionState() {
    switch (m_impl->getConnState(false)) {
        case qn::ConnectionState::Disconnected:
            return ConnectionState::Disconnected;
        case qn::ConnectionState::DnsResolving:
            return ConnectionState::DnsResolving;
        case qn::ConnectionState::Pinging:
            return ConnectionState::Pinging;
        case qn::ConnectionState::Connecting:
            return ConnectionState::Connecting;
        case qn::ConnectionState::Connected:
            return m_impl->isConnected() ? ConnectionState::Connected : ConnectionState::Authenticating;
        case qn::ConnectionState::Closing:
            return ConnectionState::Closing;
        case qn::ConnectionState::Reconnecting:
            return ConnectionState::Reconnecting;
    }
}

void NetworkManager::disconnectCentral() {
    m_impl->disconnectCentral();
}

bool NetworkManager::isConnected() const {
    return m_impl->isConnected();
}

asp::time::Duration NetworkManager::getGamePing() {
    return m_impl->getGamePing();
}

asp::time::Duration NetworkManager::getCentralPing() {
    return m_impl->getCentralPing();
}

uint32_t NetworkManager::getGameTickrate() {
    return m_impl->getGameTickrate();
}

std::vector<UserRole> NetworkManager::getAllRoles() {
    return m_impl->getAllRoles();
}

std::vector<UserRole> NetworkManager::getUserRoles() {
    return m_impl->getUserRoles();
}

std::vector<uint8_t> NetworkManager::getUserRoleIds() {
    return m_impl->getUserRoleIds();
}

std::optional<UserRole> NetworkManager::getUserHighestRole() {
    return m_impl->getUserHighestRole();
}

std::optional<UserRole> NetworkManager::findRole(uint8_t roleId) {
    return m_impl->findRole(roleId);
}

std::optional<UserRole> NetworkManager::findRole(std::string_view roleId) {
    return m_impl->findRole(roleId);
}

bool NetworkManager::isModerator() {
    return m_impl->isModerator();
}

bool NetworkManager::isAuthorizedModerator() {
    return m_impl->isAuthorizedModerator();
}

UserPermissions NetworkManager::getUserPermissions() {
    return m_impl->getUserPermissions();
}

std::optional<SpecialUserData> NetworkManager::getOwnSpecialData() {
    return m_impl->getOwnSpecialData();
}

void NetworkManager::invalidateIcons() {
    m_impl->invalidateIcons();
}

void NetworkManager::invalidateFriendList() {
    m_impl->invalidateFriendList();
}

std::optional<FeaturedLevelMeta> NetworkManager::getFeaturedLevel() {
    return m_impl->getFeaturedLevel();
}

bool NetworkManager::hasViewedFeaturedLevel() {
    return m_impl->hasViewedFeaturedLevel();
}

void NetworkManager::setViewedFeaturedLevel() {
    m_impl->setViewedFeaturedLevel();
}

void NetworkManager::queueGameEvent(OutEvent&& event) {
    m_impl->queueGameEvent(std::move(event));
}

void NetworkManager::sendQuickChat(uint32_t id) {
    m_impl->sendQuickChat(id);
}

void NetworkManager::addListener(const std::type_info& ty, void* listener, void* dtor) {
    m_impl->addListener(ty, listener, dtor);
}

void NetworkManager::removeListener(const std::type_info& ty, void* listener) {
    m_impl->removeListener(ty, listener);
}

void MessageListenerImplBase::destroy(const std::type_info& ty) {
    NetworkManager::get().removeListener(ty, this);
}

}
