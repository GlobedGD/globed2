#include "room.hpp"

RoomManager::RoomManager() {
    this->setGlobal();
}

RoomInfo& RoomManager::getInfo() {
    return roomInfo;
}

uint32_t RoomManager::getId() {
    return roomInfo.id;
}

bool RoomManager::isOwner() {
    return this->isInRoom() && roomInfo.owner == GJAccountManager::get()->m_accountID;
}

bool RoomManager::isInGlobal() {
    return roomInfo.id == 0;
}

bool RoomManager::isInRoom() {
    return !isInGlobal();
}

void RoomManager::setInfo(const RoomInfo& info) {
    roomInfo = info;
}

void RoomManager::setGlobal() {
    this->setInfo(RoomInfo {
        .id = 0,
        .owner = 0,
        .settings = {}
    });
}
