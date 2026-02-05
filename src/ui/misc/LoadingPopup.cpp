#include "LoadingPopup.hpp"

using namespace geode::prelude;

namespace globed {

bool LoadingPopup::init() {
    if (!BasePopup::init(160.f, 90.f)) return false;
    m_circle = cue::LoadingCircle::create();
    m_circle->addToLayer(m_mainLayer);
    m_circle->setPositionY(35.f);
    m_circle->setScale(0.65f);
    m_circle->fadeIn();
    this->setClosable(false);

    return true;
}

void LoadingPopup::forceClose() {
    this->setClosable(true);
    this->onClose(nullptr);
}

void LoadingPopup::setClosable(bool closable) {
    m_closeBtn->setVisible(closable);
}

void LoadingPopup::onClose(cocos2d::CCObject*) {
    if (m_closeBtn->isVisible()) {
        m_circle->fadeOut();
        Popup::onClose(nullptr);
    }
}

LoadingPopup* LoadingPopup::create() {
    auto ret = new LoadingPopup();
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

}
