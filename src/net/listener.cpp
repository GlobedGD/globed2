#include "listener.hpp"

#include <managers/settings.hpp>
#include <net/manager.hpp>
#include <util/debug.hpp>

using namespace geode::prelude;

PacketListener::~PacketListener() {}

bool PacketListener::init(packetid_t packetId, CallbackFn&& fn, CCObject* owner, int priority, bool isFinal) {
    this->callback = std::move(fn);
    this->packetId = packetId;
    this->owner = owner;
    this->priority = priority;
    this->isFinal = isFinal;

    return true;
}

void PacketListener::invokeCallback(std::shared_ptr<Packet> packet) {
    auto ownerToStr = [](CCObject* obj) -> std::string {
        if (!obj) return "<null>";

        return fmt::format("{}({})", util::debug::getTypename(obj), (void*)obj);
    };

    globed::netLog(
        "PacketListener::invokeCallback(this = {{packetId = {}, owner = {}, priority = {}, isFinal = {}}}, packet = {})",
        this->packetId, GLOBED_LAZY(ownerToStr(this->owner)), this->priority, this->isFinal, packet->getPacketId()
    );

    callback(std::move(packet));
}

PacketListener* PacketListener::create(packetid_t packetId, CallbackFn&& fn, CCObject* owner, int priority, bool isFinal) {
    auto ret = new PacketListener;
    if (ret->init(packetId, std::move(fn), owner, priority, isFinal)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}