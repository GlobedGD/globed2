#include "connection_test_popup.hpp"

#include <data/packets/client/connection.hpp>
#include <net/network_manager.hpp>
#include <util/crypto.hpp>
#include <util/rng.hpp>

using namespace geode::prelude;

bool ConnectionTestPopup::setup() {
    this->setTitle("Connection test");

    auto winSize = CCDirector::get()->getWinSize();

    Build<CCLabelBMFont>::create("", "bigFont.fnt")
        .pos(winSize.width / 2, winSize.height / 2 + m_size.height / 2 - 50.f)
        .scale(0.3f)
        .parent(m_mainLayer)
        .store(statusLabel);

    auto& nm = NetworkManager::get();
    nm.addListener<ConnectionTestResponsePacket>([this] (ConnectionTestResponsePacket* packet) {
        if (packet->uid != this->uid) {
            FLAlertLayer::create("Error", "Test failed. Server sent invalid unique packet ID.", "Ok")->show();
            this->onClose(this);
            return;
        }
        // discard the data

        this->failedAttempts = 0;
        this->nextStep();
    });

    currentSize = 512;

    this->nextStep();

    this->schedule(schedule_selector(ConnectionTestPopup::checkForUpdates), 0.25f);

    return true;
}

void ConnectionTestPopup::checkForUpdates(float) {
    if (util::time::systemNow() - lastPacket > util::time::millis(2500)) {
        currentAttempt--;
        failedAttempts++;
        log::warn("failed attempt! size: {}, current attempt: {}", currentSize, currentAttempt);

        // if no response 3 times in a row, abort
        if (failedAttempts > 2) {
            FLAlertLayer::create("Error", fmt::format("Test failed with packet size = {}. No response received after 3 attempts.", currentSize), "Ok")->show();
            this->closeDelayed();
            return;
        }

        this->nextStep();
    }
}

void ConnectionTestPopup::nextStep() {
    if (currentAttempt > 4) {
        currentAttempt = 0;
        log::debug("upgrading from {} to {}", currentSize, currentSize * 1.5);
        currentSize *= 1.5;
    }

    if (currentSize > 55555) {
        FLAlertLayer::create("Success", "Connection test succeeded. No issues found.", "Ok")->show();
        this->closeDelayed();
        return;
    }

    auto& rng = util::rng::Random::get();
    uid = rng.generate<int>();

    auto data = util::crypto::secureRandom(currentSize);

    auto& nm = NetworkManager::get();
    nm.send(ConnectionTestPacket::create(uid, std::move(data)));
    lastPacket = util::time::systemNow();

    currentAttempt++;

    statusLabel->setString(fmt::format("Attempt {} with size = {}", currentAttempt, currentSize).c_str());
}

void ConnectionTestPopup::onClose(cocos2d::CCObject* obj) {
    Popup::onClose(obj);

    auto& nm = NetworkManager::get();
    nm.removeListener<ConnectionTestResponsePacket>();
    nm.suppressUnhandledFor<ConnectionTestResponsePacket>(util::time::seconds(1));
}

void ConnectionTestPopup::closeDelayed() {
    // this is needed because we cant call NetworkManager::removeListener inside of a listener callback.
    Loader::get()->queueInMainThread([this] {
        this->onClose(this);
    });
}

ConnectionTestPopup* ConnectionTestPopup::create() {
    auto ret = new ConnectionTestPopup;
    if (ret->init(POPUP_WIDTH, POPUP_HEIGHT)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}