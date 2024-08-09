#include "server_test_popup.hpp"

#include <Geode/utils/web.hpp>

#include "add_server_popup.hpp"
#include <net/manager.hpp>
#include <managers/error_queues.hpp>
#include <util/net.hpp>
#include <util/time.hpp>
#include <util/format.hpp>

using namespace geode::prelude;

bool ServerTestPopup::setup(const std::string_view url, AddServerPopup* parent) {
    this->parent = parent;
    this->setTitle("Testing server");

    auto winSize = cocos2d::CCDirector::sharedDirector()->getWinSize();

    Build<CCLabelBMFont>::create("Attempting to connect..", "bigFont.fnt")
        .pos(winSize.width / 2, winSize.height / 2 + m_size.height / 2 - 50.f)
        .scale(0.35f)
        .parent(m_mainLayer);

    auto request = WebRequestManager::get().testServer(url);

    requestListener.bind(this, &ServerTestPopup::requestCallback);
    requestListener.setFilter(std::move(request));

    return true;
}

void ServerTestPopup::requestCallback(typename WebRequestManager::Event* event) {
    if (!event || !event->getValue()) return;

    auto evalue = std::move(*event->getValue());

    if (!evalue.ok()) {
        auto error = evalue.getError();

        this->parent->onTestFailure(error);
        this->onClose(this);
        return;
    }

    auto resp = evalue.text().unwrapOrDefault();

    int protocol = util::format::parse<int>(resp).value_or(0);

    if (!NetworkManager::get().isProtocolSupported(protocol)) {
        this->parent->onTestFailure(fmt::format(
            "Failed to add the server due to version mismatch. Client protocol version: v{}, server: v{}",
            NetworkManager::get().getUsedProtocol(),
            protocol
        ));
    } else {
        this->parent->onTestSuccess();
    }

    this->onClose(this);
}

ServerTestPopup::~ServerTestPopup() {
    this->cancelRequest();
}

void ServerTestPopup::onTimeout() {
    this->cancelRequest();

    this->parent->onTestFailure("Failed to add the server: timed out while waiting for a response.");
    this->onClose(this);
}

void ServerTestPopup::cancelRequest() {
    requestListener.getFilter().cancel();
}

ServerTestPopup* ServerTestPopup::create(const std::string_view url, AddServerPopup* parent) {
    auto ret = new ServerTestPopup;
    if (ret->init(POPUP_WIDTH, POPUP_HEIGHT, url, parent)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}
