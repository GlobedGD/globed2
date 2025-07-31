#pragma once

#include <ui/BasePopup.hpp>

#include <cue/LoadingCircle.hpp>

namespace globed {

class LoadingPopup : public BasePopup<LoadingPopup> {
public:
    using BasePopup::setTitle;

    static const cocos2d::CCSize POPUP_SIZE;
    void setClosable(bool closable);
    void forceClose();

protected:
    cue::LoadingCircle* m_circle;

    bool setup() override;
    void onClose(cocos2d::CCObject*) override;
};

}