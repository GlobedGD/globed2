#include "admin.hpp"

bool AdminManager::authorized() {
    return authorized_;
}

void AdminManager::setAuthorized(ComputedRole&& role) {
    authorized_ = true;
    role = std::move(role);
}

void AdminManager::deauthorize() {
    authorized_ = false;
    role = {};
}

ComputedRole& AdminManager::getRole() {
    return role;
}