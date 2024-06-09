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

    auto request = web::WebRequest()
        .userAgent(util::net::webUserAgent())
        .timeout(util::time::seconds(5))
        .get(fmt::format("{}/version", url))
        .map([this](web::WebResponse* response) -> Result<std::string, std::string> {
            GLOBED_UNWRAP_INTO(response->string(), auto resptext);

            if (resptext.empty()) {
                return Err(fmt::format("server sent an empty response with code {}", response->code()));
            }

            if (response->ok()) {
                return Ok(resptext);
            } else {
                return Err("<cy>" + util::format::formatErrorMessage(resptext) + "</c>");
            }

        }, [](web::WebProgress*) -> std::monostate {
            return {};
        });

    requestListener.bind(this, &ServerTestPopup::requestCallback);
    requestListener.setFilter(std::move(request));

    return true;
}

void ServerTestPopup::requestCallback(typename geode::Task<Result<std::string, std::string>>::Event* event) {
    if (!event || !event->getValue()) return;

    auto evalue = event->getValue();

    if (!evalue->isOk()) {
        std::string message = evalue->unwrapErr();

        this->parent->onTestFailure(message);
        this->onClose(this);
        return;
    }

    auto resp = evalue->unwrap();

    int protocol = util::format::parse<int>(resp).value_or(0);

    if (protocol != NetworkManager::get().getUsedProtocol()) {
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
