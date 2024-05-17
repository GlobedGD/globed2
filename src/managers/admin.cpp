#include "admin.hpp"

bool AdminManager::authorized() {
    return authorized_;
}

void AdminManager::setAuthorized(ComputedRole&& role, std::vector<ServerRole>&& allRoles) {
    authorized_ = true;
    role = std::move(role);
    allRoles = std::move(allRoles);
}

void AdminManager::deauthorize() {
    authorized_ = false;
    role = {};
}

ComputedRole& AdminManager::getRole() {
    return role;
}

std::vector<ServerRole>& AdminManager::getAllRoles() {
    return allRoles;
}
