#include "listener.hpp"

#include <net/network_manager.hpp>

using namespace geode::prelude;

PacketListener::~PacketListener() {
    auto& nm = NetworkManager::get();
    nm.unregisterPacketListener(packetId, this);
    nm.suppressUnhandledFor(packetId, util::time::seconds(3));
}

bool PacketListener::init(packetid_t packetId, CallbackFn&& fn, CCObject* owner, int priority, bool mainThread, bool isFinal) {
    this->callback = std::move(fn);
    this->packetId = packetId;
    this->owner = owner;
    this->priority = priority;
    this->mainThread = mainThread;
    this->isFinal = isFinal;

    return true;
}

void PacketListener::invokeCallback(std::shared_ptr<Packet> packet) {
    // this might not seem like it but it must be kept oustide of the `if { ... }`
    Ref<CCObject> ownerRef(owner);

    if (mainThread) {
        Loader::get()->queueInMainThread([this, ownerRef = std::move(ownerRef), packet = std::move(packet)] {
            this->callback(packet);
        });
    } else {
        callback(packet);
    }
}

PacketListener* PacketListener::create(packetid_t packetId, CallbackFn&& fn, CCObject* owner, int priority, bool mainThread, bool isFinal) {
    auto ret = new PacketListener;
    if (ret->init(packetId, std::move(fn), owner, priority, mainThread, isFinal)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}