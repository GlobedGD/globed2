#include "LoadingPopup.hpp"

using namespace geode::prelude;

namespace globed {

bool LoadingPopup::init() {
    if (!BasePopup::init(170.f, 90.f)) return false;

    m_circle = cue::LoadingCircle::create();
    m_circle->addToLayer(m_mainLayer);
    m_circle->setPositionY(38.f);
    m_circle->setScale(0.65f);
    m_circle->fadeIn();
    this->setClosable(false);

    auto closeSpr = m_closeBtn->getNormalImage();
    closeSpr->setScale(closeSpr->getScale() * 0.8f);

    return true;
}

void LoadingPopup::setTitle(geode::ZStringView title) {
    BasePopup::setTitle(title, "goldFont.fnt", 0.65f, 14.f);
    m_title->limitLabelWidth(m_size.width * 0.9f, 0.65f, 0.2f);
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
