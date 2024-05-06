#pragma once

#include <defs/geode.hpp>
#include <data/packets/packet.hpp>

class PacketListener : public cocos2d::CCObject {
public:
    using CallbackFn = std::function<void(std::shared_ptr<Packet>)>;

    ~PacketListener();

    static PacketListener* create(packetid_t packetId, CallbackFn&& fn, cocos2d::CCObject* owner);

    void invokeCallback(std::shared_ptr<Packet> packet);

    cocos2d::CCObject* owner;

private:
    CallbackFn callback;
    packetid_t packetId;

    bool init(packetid_t packetId, CallbackFn&& fn, cocos2d::CCObject* owner);
};
