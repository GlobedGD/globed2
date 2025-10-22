#include "InputPopup.hpp"

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

bool InputPopup::setup(const char* font) {
    if (!BasePopup::setup(font)) {
        return false;
    }

    m_input = TextInput::create(0.f, "", font);
    m_mainLayer->addChild(m_input);

    m_submitBtn = Build(ButtonSprite::create("Submit", "bigFont.fnt", "GJ_button_01.png", 0.7f))
        .scale(0.8f)
        .intoMenuItem([this] {
            if (m_callback) {
                m_callback({
                    .text = m_input->getString(),
                    .cancelled = false
                });
            }

            this->onClose(this);
        })
        .scaleMult(1.1f)
        .parent(m_buttonMenu);

    return true;
}

void InputPopup::show() {
    if (m_mySize.width == 0.f) {
        // try to automatically determine the size based on other parameters, although not very good
        m_mySize.width = std::clamp(m_maxCharCount * 10.f, 160.f, 400.f);
    }

    m_mySize.height = 120.f;

    m_input->setWidth(m_mySize.width - 30.f);
    m_input->setPosition(m_mySize.width / 2.f, m_mySize.height / 2.f + 5.f);
    m_input->setString(m_defaultText);
    m_submitBtn->setPosition(m_mySize.width / 2.f, 24.f);
    m_mainLayer->setContentSize(m_mySize); // evil
    m_mainLayer->updateLayout();

    BasePopup::show();
}

void InputPopup::setPlaceholder(const std::string& placeholder) {
    m_input->setPlaceholder(placeholder);
}

void InputPopup::setFilter(const std::string& allowedChars) {
    m_input->setFilter(allowedChars);
}

void InputPopup::setCommonFilter(geode::CommonFilter filter) {
    m_input->setCommonFilter(filter);
}

void InputPopup::setMaxCharCount(size_t length) {
    m_maxCharCount = length;
    m_input->setMaxCharCount(length);
}

void InputPopup::setPasswordMode(bool password) {
    m_input->setPasswordMode(password);
}

void InputPopup::setDefaultText(const std::string& text) {
    m_defaultText = text;
}

void InputPopup::setCallback(std23::move_only_function<void(InputPopupOutcome)> callback) {
    m_callback = std::move(callback);
}

void InputPopup::setWidth(float width) {
    m_mySize.width = width;
}

void InputPopup::onClose(cocos2d::CCObject*) {
    if (m_callback) {
        m_callback({
            .text = m_input->getString(),
            .cancelled = true
        });
    }

    Popup::onClose(nullptr);
}

}
