#include "listener.hpp"

#include <net/network_manager.hpp>

using namespace geode::prelude;

PacketListener::~PacketListener() {
    auto& nm = NetworkManager::get();
    nm.unregisterPacketListener(packetId, this);
    nm.suppressUnhandledFor(packetId, util::time::seconds(3));
}

bool PacketListener::init(packetid_t packetId, CallbackFn&& fn) {
    this->callback = std::move(fn);
    this->packetId = packetId;

    return true;
}

void PacketListener::invokeCallback(std::shared_ptr<Packet> packet) {
    callback(packet);
}

PacketListener* PacketListener::create(packetid_t packetId, CallbackFn&& fn) {
    auto ret = new PacketListener;
    if (ret->init(packetId, std::move(fn))) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}