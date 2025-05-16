#include "frag_calibration_popup.hpp"

#include <data/packets/client/connection.hpp>
#include <net/manager.hpp>
#include <managers/settings.hpp>
#include <managers/popup.hpp>
#include <util/crypto.hpp>
#include <util/rng.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;
using namespace asp::time;

bool FragmentationCalibrationPopup::setup() {
    this->setTitle("Connection test");

    auto rlayout = util::ui::getPopupLayoutAnchored(m_size);

    Build<CCLabelBMFont>::create("", "bigFont.fnt")
        .pos(rlayout.fromTop(50.f))
        .scale(0.3f)
        .parent(m_mainLayer)
        .store(statusLabel);

    auto& nm = NetworkManager::get();
    nm.addListener<ConnectionTestResponsePacket>(this, [this] (std::shared_ptr<ConnectionTestResponsePacket> packet) {
        if (packet->uid != this->uid) {
            auto newFragLimit = TEST_PACKET_SIZES[currentSizeIdx - 1];
            auto& settings = GlobedSettings::get();
            settings.globed.fragmentationLimit = newFragLimit;

            PopupManager::get().alertFormat(
                "Error",
                "Test failed fatally. Server sent invalid unique packet ID. Setting maximum packet size to {}.",
                newFragLimit
            ).showInstant();

            this->closeDelayed();
            return;
        }
        // discard the data

        this->failedAttempts = 0;
        this->nextStep();
    });

    currentSizeIdx = 0;
    currentSize = TEST_PACKET_SIZES[currentSizeIdx];

    this->nextStep();

    this->schedule(schedule_selector(FragmentationCalibrationPopup::checkForUpdates), 0.25f);

    return true;
}

void FragmentationCalibrationPopup::checkForUpdates(float) {
    if (lastPacket.elapsed().millis() > 2500) {
        currentAttempt--;
        failedAttempts++;
        log::warn("failed attempt! size: {}, current attempt: {}", currentSize, currentAttempt);

        // if no response 3 times in a row, abort
        if (failedAttempts > 2) {
            auto newFragLimit = TEST_PACKET_SIZES[currentSizeIdx - 1];
            PopupManager::get().alertFormat(
                "Notice",
                "Test ended early with packet size = {}. Setting the maximum packet size to {}. "
                "Reconnect to the server in order to see any change.",
                currentSize, newFragLimit
            ).showInstant();

            this->closeDelayed();
            return;
        }

        this->nextStep();
    }
}

void FragmentationCalibrationPopup::nextStep() {
    if (currentAttempt > 4) {
        currentAttempt = 0;
        auto newSize = TEST_PACKET_SIZES[++currentSizeIdx];
        currentSize = newSize;
    }

    if (currentSize == 0) {
        // calibration was fully successful, user can send up to 64kb
        PopupManager::get().alert("Success", "Connection test succeeded. No issues found.").showInstant();
        this->closeDelayed();
        return;
    }

    auto& rng = util::rng::Random::get();
    uid = rng.generate<int>();

    auto data = util::crypto::secureRandom(currentSize);

    auto& nm = NetworkManager::get();
    if (!nm.established()) {
        PopupManager::get().alert("Failed", "Fatal failure. Disconnected during the test.").showInstant();
        this->closeDelayed();
        return;
    }

    nm.send(ConnectionTestPacket::create(uid, std::move(data)));
    lastPacket = SystemTime::now();

    currentAttempt++;

    statusLabel->setString(fmt::format("Attempt {} with size = {}", currentAttempt, currentSize).c_str());
}

void FragmentationCalibrationPopup::onClose(cocos2d::CCObject* obj) {
    Popup::onClose(obj);

    if (currentSizeIdx > 0) {
        auto newFragLimit = TEST_PACKET_SIZES[currentSizeIdx - 1];
        if (newFragLimit > 1300) {
            auto& settings = GlobedSettings::get();
            settings.globed.fragmentationLimit = newFragLimit;
        }
    }
}

void FragmentationCalibrationPopup::closeDelayed() {
    // this is needed because we cant call NetworkManager::removeListener inside of a listener callback.
    Loader::get()->queueInMainThread([this] {
        this->onClose(this);
    });
}

FragmentationCalibrationPopup* FragmentationCalibrationPopup::create() {
    auto ret = new FragmentationCalibrationPopup;
    if (ret->initAnchored(POPUP_WIDTH, POPUP_HEIGHT)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}
