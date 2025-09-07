#pragma once

#include <globed/core/data/Messages.hpp>

#include <Geode/Geode.hpp>
#include <cue/PlayerIcon.hpp>

namespace globed {

class InviteNotification : public cocos2d::CCLayer {
public:
    static InviteNotification* create(const msg::InvitedMessage& msg);

private:
    bool init(const msg::InvitedMessage& msg);
    void removeFromParent() override;
};

}
