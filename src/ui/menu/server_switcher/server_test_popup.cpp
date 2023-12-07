#include "server_test_popup.hpp"

#include <UIBuilder.hpp>

#include "add_server_popup.hpp"
#include <net/network_manager.hpp>
#include <managers/error_queues.hpp>
#include <util/net.hpp>
#include <util/time.hpp>
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

    sentRequestHandle = GHTTPRequest::get(fmt::format("{}/version", url))
        .userAgent(util::net::webUserAgent())
        .timeout(util::time::secs(5))
        .then([this](const GHTTPResponse& resp) {
            sentRequestHandle = std::nullopt;

            if (resp.anyfail()) {
                std::string error;

                if (resp.resCode == CURLE_OPERATION_TIMEDOUT) {
                    error = "Failed to contact the server, no response was received after 5 seconds.";
                } else {
                    auto msg = resp.anyfailmsg();
                    if (msg.empty()) {
                        error = "Error retrieving data from the server: server sent an empty response.";
                    } else {
                        error = "Error retrieving data from the server: <cy>" + util::formatting::formatErrorMessage(msg) + "</c>";
                    }
                }

                this->parent->onTestFailure(error);
            } else {
                auto response = resp.response;
                int protocol = 0;
#ifdef GLOBED_ANDROID
                // this is such a meme im crying
                std::istringstream iss(response);
                iss >> protocol;
                if (iss.fail() || !iss.eof()) {
                    protocol = 0;
                }
#else
                std::from_chars(response.data(), response.data() + response.size(), protocol);
#endif

                if (protocol != NetworkManager::PROTOCOL_VERSION) {
                    this->parent->onTestFailure(fmt::format(
                        "Failed to add the server due to version mismatch. Client protocol version: v{}, server: v{}",
                        NetworkManager::PROTOCOL_VERSION,
                        protocol
                    ));
                } else {
                    this->parent->onTestSuccess();
                }
            }

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
        sentRequestHandle->discardResult();
        sentRequestHandle = std::nullopt;
    }
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
