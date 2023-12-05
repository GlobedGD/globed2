#include "server_test_popup.hpp"

#include <UIBuilder.hpp>

#include "add_server_popup.hpp"
#include <net/network_manager.hpp>
#include <managers/error_queues.hpp>
#include <util/net.hpp>
#include <util/formatting.hpp>

using namespace geode::prelude;

bool ServerTestPopup::setup(const std::string& url, AddServerPopup* parent) {
    this->parent = parent;
    this->setTitle("Testing server");

    auto winSize = cocos2d::CCDirector::sharedDirector()->getWinSize();

    Build<CCLabelBMFont>::create("Attempting to connect..", "bigFont.fnt")
        .pos(winSize.width / 2, winSize.height / 2 + m_size.height / 2 - 50.f)
        .scale(0.35f)
        .parent(m_mainLayer);

    sentRequestHandle = web::AsyncWebRequest()
        .userAgent(util::net::webUserAgent())
        .fetch(fmt::format("{}/version", url)).text()
        .then([this](const std::string& response) {
            sentRequestHandle = std::nullopt;
            if (timeoutSequence) {
                this->stopAction(timeoutSequence);
                timeoutSequence = nullptr;
            }

            int protocol = 0;
            std::from_chars(response.data(), response.data() + response.size(), protocol);

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
        .expect([this](const std::string& error) {
            // why the fuck does geode call the error if the request was cancelled????
            if (error.find("was aborted by an") != std::string::npos) {
                return;
            }

            sentRequestHandle = std::nullopt;
            if (timeoutSequence) {
                this->stopAction(timeoutSequence);
                timeoutSequence = nullptr;
            }

            if (error.empty()) {
                this->parent->onTestFailure("Failed to make a request to the server: server sent empty response.");
            } else {
                this->parent->onTestFailure("Failed to make a request to the server: <cy>" + util::formatting::formatErrorMessage(error) + "</c>");
            }
            this->onClose(this);
        })
        .send();

    // cancel the request after 5 seconds if no response
    timeoutSequence = CCSequence::create(
        CCDelayTime::create(5.0f),
        CCCallFunc::create(this, callfunc_selector(ServerTestPopup::onTimeout)),
        nullptr
    );

    this->runAction(timeoutSequence);

    return true;
}

ServerTestPopup::~ServerTestPopup() {
    if (sentRequestHandle.has_value()) {
        sentRequestHandle->get()->cancel();
    }
}

void ServerTestPopup::onTimeout() {
    if (timeoutSequence) {
        this->stopAction(timeoutSequence);
        timeoutSequence = nullptr;
    }

    if (sentRequestHandle.has_value()) {
        sentRequestHandle->get()->cancel();
    }

    this->parent->onTestFailure("Failed to add the server: timed out while waiting for a response.");
    this->onClose(this);
}

ServerTestPopup* ServerTestPopup::create(const std::string& url, AddServerPopup* parent) {
    auto ret = new ServerTestPopup;
    if (ret && ret->init(POPUP_WIDTH, POPUP_HEIGHT, url, parent)) {
        ret->autorelease();
        return ret;
    }

    CC_SAFE_DELETE(ret);
    return nullptr;
}
