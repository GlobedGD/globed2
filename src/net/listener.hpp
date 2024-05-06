#pragma once

#include <defs/geode.hpp>
#include <data/packets/packet.hpp>

class PacketListener : public cocos2d::CCObject {
public:
    using CallbackFn = std::function<void(std::shared_ptr<Packet>)>;

    ~PacketListener();

    static PacketListener* create(packetid_t packetId, CallbackFn&& fn);

    void invokeCallback(std::shared_ptr<Packet> packet);

private:
    CallbackFn callback;
    packetid_t packetId;

    bool init(packetid_t packetId, CallbackFn&& fn);
};
