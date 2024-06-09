#include "listener.hpp"

#include <net/manager.hpp>

using namespace geode::prelude;

PacketListener::~PacketListener() {
    auto& nm = NetworkManager::get();
    nm.unregisterPacketListener(packetId, this, true);
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
    if (owner) {
        owner->retain();
    }

    if (mainThread) {
        Loader::get()->queueInMainThread([this, packet = std::move(packet)] {
            this->callback(packet);
            if (owner) this->owner->release();
        });
    } else {
        callback(packet);
        if (owner) this->owner->release();
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