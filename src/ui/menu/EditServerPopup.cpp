#include "EditServerPopup.hpp"

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

const CCSize EditServerPopup::POPUP_SIZE { 380.f, 240.f };

bool EditServerPopup::setup(bool adding, const std::string& name, const std::string& url) {
    this->setTitle(adding ? "Add a new server" : "Edit server");

    Build<CCLabelBMFont>::create("Server Name", "bigFont.fnt")
        .scale(0.3f)
        .pos(this->fromTop(60.f))
        .parent(m_mainLayer);

    m_nameInput = Build(TextInput::create(320.f, "Server Name"))
        .pos(this->fromTop(80.f))
        .parent(m_mainLayer);

    m_nameInput->setString(name);

    Build<CCLabelBMFont>::create("Server URL", "bigFont.fnt")
        .scale(0.3f)
        .pos(this->fromTop(120.f))
        .parent(m_mainLayer);

    m_urlInput = Build(TextInput::create(320.f, "Server URL", "chatFont.fnt"))
        .pos(this->fromTop(140.f))
        .parent(m_mainLayer);

    m_urlInput->setCommonFilter(CommonFilter::Any);
    m_urlInput->setString(url);
    m_urlInput->setMaxCharCount(80);

    if (!adding) {
        m_nameInput->setString(name);
        m_urlInput->setString(url);
    }

    Build<ButtonSprite>::create(adding ? "Create" : "Save", "bigFont.fnt", "GJ_button_01.png", 0.8f)
        .scale(0.9f)
        .intoMenuItem([this] {
            if (m_callback) {
                m_callback(m_nameInput->getString(), m_urlInput->getString());
                this->onClose(nullptr);
            }
        })
        .id("add-btn")
        .pos(this->fromBottom(28.f))
        .parent(m_buttonMenu);

    return true;
}

void EditServerPopup::setCallback(Callback&& fn) {
    m_callback = std::move(fn);
}

}