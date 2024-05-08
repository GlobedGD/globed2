#include "listener.hpp"

#include <net/network_manager.hpp>

using namespace geode::prelude;

PacketListener::~PacketListener() {
    auto& nm = NetworkManager::get();
    nm.unregisterPacketListener(packetId, this);
    nm.suppressUnhandledFor(packetId, util::time::seconds(3));
}

bool PacketListener::init(packetid_t packetId, CallbackFn&& fn, CCObject* owner, bool overrideBuiltin) {
    this->callback = std::move(fn);
    this->packetId = packetId;
    this->owner = owner;
    this->overrideBuiltin = overrideBuiltin;

    return true;
}

void PacketListener::invokeCallback(std::shared_ptr<Packet> packet) {
    callback(packet);
}

PacketListener* PacketListener::create(packetid_t packetId, CallbackFn&& fn, CCObject* owner, bool overrideBuiltin) {
    auto ret = new PacketListener;
    if (ret->init(packetId, std::move(fn), owner, overrideBuiltin)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}