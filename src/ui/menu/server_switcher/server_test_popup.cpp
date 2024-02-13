#include "server_test_popup.hpp"

#include "add_server_popup.hpp"
#include <net/network_manager.hpp>
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

    sentRequestHandle = web::AsyncWebRequest()
        .userAgent(util::net::webUserAgent())
        .timeout(util::time::seconds(5))
        .get(fmt::format("{}/version", url))
        .text()
        .then([this](std::string& resp) {
            sentRequestHandle = std::nullopt;

            int protocol = util::format::parse<int>(resp).value_or(0);

            if (protocol != NetworkManager::PROTOCOL_VERSION) {
                this->parent->onTestFailure(fmt::format(
                    "Failed to add the server due to version mismatch. Client protocol version: v{}, server: v{}",
                    NetworkManager::PROTOCOL_VERSION,
                    protocol
                ));
            } else {
                this->parent->onTestSuccess();
            }

            this->onClose(this);
        })
        .expect([this](std::string error, int statusCode) {
            sentRequestHandle = std::nullopt;

            if (error.empty()) {
                error = fmt::format("Error retrieving data from the server: server sent an empty response with code {}.", statusCode);
            } else {
                error = "Error retrieving data from the server: <cy>" + util::format::formatErrorMessage(error) + "</c>";
            }

            this->parent->onTestFailure(error);
            this->onClose(this);
        })
        .send();

    return true;
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
    if (sentRequestHandle.has_value()) {
        sentRequestHandle->get()->cancel();
        sentRequestHandle = std::nullopt;
    }
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
