#pragma once

#include <ui/BasePopup.hpp>

#include <cue/LoadingCircle.hpp>

namespace globed {

class LoadingPopup : public BasePopup {
public:
    static LoadingPopup* create();

    void setClosable(bool closable);
    void forceClose();
    void setTitle(geode::ZStringView title);

protected:
    cue::LoadingCircle* m_circle;

    bool init() override;
    void onClose(cocos2d::CCObject*) override;
};

}