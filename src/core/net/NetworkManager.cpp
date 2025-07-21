#include <globed/core/net/NetworkManager.hpp>

#include "NetworkManagerImpl.hpp"

using namespace geode::prelude;

namespace globed {

NetworkManager::NetworkManager() : m_impl(std::make_unique<NetworkManagerImpl>()) {}

NetworkManager::~NetworkManager() {
    log::debug("goodbye from networkmanager!");
}

Result<> NetworkManager::connectCentral(std::string_view url) {
    return m_impl->connectCentral(url);
}

}