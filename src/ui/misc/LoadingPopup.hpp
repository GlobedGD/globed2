#pragma once

#include <ui/BasePopup.hpp>

#include <cue/LoadingCircle.hpp>

namespace globed {

class LoadingPopup : public BasePopup {
public:
    using BasePopup::setTitle;

    static LoadingPopup* create();

    void setClosable(bool closable);
    void forceClose();

protected:
    cue::LoadingCircle* m_circle;

    bool init() override;
    void onClose(cocos2d::CCObject*) override;
};

}