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

ccColor3B NetworkManager::latencyToColor(uint64_t ms) {
    std::array breakpoints = {
        std::make_pair(0, ccColor3B{0, 255, 0}),
        std::make_pair(50, ccColor3B{144, 238, 144}),
        std::make_pair(125, ccColor3B{255, 255, 0}),
        std::make_pair(200, ccColor3B{255, 165, 0}),
        std::make_pair(300, ccColor3B{255, 0, 0}),
    };

    ms = std::clamp<uint64_t>(ms, breakpoints.front().first, breakpoints.back().first);

    for (size_t i = 0; i < breakpoints.size() - 1; i++) {
        auto& [lo_val, lo_color] = breakpoints[i];
        auto& [hi_val, hi_color] = breakpoints[i + 1];

        if (ms > (uint64_t)hi_val) {
            continue;
        }

        float t = (float)(ms - lo_val) / (float)(hi_val - lo_val);
        uint8_t r = std::round(std::lerp((float)lo_color.r, (float)hi_color.r, t));
        uint8_t g = std::round(std::lerp((float)lo_color.g, (float)hi_color.g, t));
        uint8_t b = std::round(std::lerp((float)lo_color.b, (float)hi_color.b, t));

        return ccColor3B{r, g, b};
    }

    // should never reach here
    std::unreachable();
}

}
