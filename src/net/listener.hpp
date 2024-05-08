#pragma once

#include <defs/geode.hpp>
#include <data/packets/packet.hpp>

class PacketListener : public cocos2d::CCObject {
public:
    using CallbackFn = std::function<void(std::shared_ptr<Packet>)>;

    ~PacketListener();

    static PacketListener* create(packetid_t packetId, CallbackFn&& fn, cocos2d::CCObject* owner, bool overrideBuiltin);

    void invokeCallback(std::shared_ptr<Packet> packet);

    cocos2d::CCObject* owner;
    bool overrideBuiltin;
    packetid_t packetId;

private:
    CallbackFn callback;

    bool init(packetid_t packetId, CallbackFn&& fn, cocos2d::CCObject* owner, bool overrideBuiltin);
};
