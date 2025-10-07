#pragma once

#include <globed/prelude.hpp>
#include <ui/BasePopup.hpp>

namespace globed {

class SendFeaturePopup : public BasePopup<SendFeaturePopup, GJGameLevel*> {
public:
    static const CCSize POPUP_SIZE;

private:
    GJGameLevel* m_level;

    bool setup(GJGameLevel* level);
};

}