#include "server_test_popup.hpp"

#include <Geode/utils/web.hpp>

#include "add_server_popup.hpp"
#include <net/manager.hpp>
#include <managers/error_queues.hpp>
#include <util/net.hpp>
#include <util/time.hpp>
#include <util/format.hpp>

using namespace geode::prelude;

bool ServerTestPopup::setup(std::string_view url, AddServerPopup* parent) {
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

struct VersionCheckResponse {
    uint16_t pmin;
    uint16_t pmax;
    std::string gdmin;
    std::string globedmin;
};

template<>
struct matjson::Serialize<VersionCheckResponse> {
	static VersionCheckResponse from_json(const matjson::Value& v) {
        return VersionCheckResponse {
            .pmin = static_cast<uint16_t>(v["pmin"].as_int()),
            .pmax = static_cast<uint16_t>(v["pmax"].as_int()),
            .gdmin = v["gdmin"].as_string(),
            .globedmin = v["globedmin"].as_string(),
        };
    }

	static matjson::Value to_json(const VersionCheckResponse& obj) {
        GLOBED_UNIMPL("serializer for VersionCheckResponse");
    }

	static bool is_json(const matjson::Value& obj) {
        return obj.contains("pmin") && obj.contains("pmax") && obj.contains("gdmin") && obj.contains("globedmin")
            && obj["pmin"].is_number() && obj["pmax"].is_number() && obj["gdmin"].is_number() && obj["globedmin"].is_number();
    }
};

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

    std::string errormsg;
    auto parseresult = matjson::parse(resp, errormsg);

    if (!parseresult || !parseresult->is<VersionCheckResponse>()) {
        if (errormsg.empty()) {
            errormsg = "unexpected structure";
        }

        log::warn("Bogus server response for versioncheck: {}", resp);
        this->parent->onTestFailure(fmt::format("Failed to parse message returned by the server: {}", errormsg));
        this->onClose(this);
        return;
    }

    auto respv = parseresult->as<VersionCheckResponse>();

    bool anySupport = false;
    for (int i = respv.pmin; i <= respv.pmax; i++) {
        if (NetworkManager::get().isProtocolSupported(i)) {
            anySupport = true;
            break;
        }
    }

    if (!anySupport) {
        this->parent->onTestFailure(fmt::format(
            "Failed to add the server due to version mismatch. Client protocol version: v{}, server expects from v{} to v{}",
            NetworkManager::get().getUsedProtocol(),
            respv.pmin, respv.pmax
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

ServerTestPopup* ServerTestPopup::create(std::string_view url, AddServerPopup* parent) {
    auto ret = new ServerTestPopup;
    if (ret->init(POPUP_WIDTH, POPUP_HEIGHT, url, parent)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}
