#include "signup_popup.hpp"

#include <Geode/utils/web.hpp>
#include <managers/error_queues.hpp>
#include <managers/central_server.hpp>
#include <managers/game_server.hpp>
#include <managers/account.hpp>
#include <net/manager.hpp>
#include <util/net.hpp>
#include <util/format.hpp>
#include <util/crypto.hpp>

using namespace geode::prelude;

bool GlobedSignupPopup::setup() {
    this->setTitle("Authentication");
    m_closeBtn->setVisible(false);

    auto& csm = CentralServerManager::get();
    auto& am = GlobedAccountManager::get();

    auto activeServer = csm.getActive();
    if (!activeServer.has_value()) {
        return false;
    }

    auto winSize = cocos2d::CCDirector::sharedDirector()->getWinSize();

    Build<CCLabelBMFont>::create("Requesting challenge..", "bigFont.fnt")
        .pos(winSize.width / 2, winSize.height / 2 + m_size.height / 2 - 50.f)
        .scale(0.35f)
        .store(statusMessage)
        .parent(m_mainLayer);

    auto request = WebRequestManager::get().challengeStart();
    createListener.bind(this, &GlobedSignupPopup::createCallback);
    createListener.setFilter(std::move(request));

    return true;
}

void GlobedSignupPopup::createCallback(typename WebRequestManager::Event* event) {
    if (!event || !event->getValue()) return;

    auto evalue = std::move(*event->getValue());

    if (!evalue.ok()) {
        std::string message = evalue.getError();

        log::warn("error creating challenge");
        log::warn("{}", message);

        this->onFailure("Creating challenge failed: <cy>" + message + "</c>");

        return;
    }

    auto resptext = evalue.text().unwrapOrDefault();

    auto parts = util::format::split(resptext, ":");
    if (parts.size() != 3) {
        this->onFailure("Creating challenge failed: <cy>response does not consist of 3 parts</c>");
        return;
    }

    // we accept -1 as the default
    int accountId = util::format::parse<int>(parts[0]).value_or(-1);

    this->onChallengeCreated(accountId, parts[1], parts[2]);
}

void GlobedSignupPopup::onChallengeCreated(int accountId, const std::string_view chtoken, const std::string_view pubkey) {
    std::string answer;

    try {
        auto decodedChallenge = util::crypto::base64Decode(chtoken);
        auto cryptoKey = util::crypto::base64Decode(pubkey);
        SecretBox box(cryptoKey);
        answer = box.decryptToString(decodedChallenge);
    } catch (const std::exception& e) {
        log::warn("failed to complete challenge: {}", e.what());
        this->onFailure(fmt::format("Failed to complete challenge: <cy>{}</c>", e.what()));
        return;
    }

    if (accountId == -1) {
        // skip the account verification, server has it disabled
        this->onChallengeCompleted(answer);
    } else {
        statusMessage->setString("Uploading results..");
        storedAuthcode = answer;
        storedAccountId = accountId;
        GameLevelManager::sharedState()->m_uploadMessageDelegate = this;
        GameLevelManager::sharedState()->uploadUserMessage(storedAccountId, fmt::format("##c## {}", answer), "globed verification test, this message can be deleted");
    }
}

void GlobedSignupPopup::uploadMessageFinished(int) {
    GameLevelManager::sharedState()->m_uploadMessageDelegate = nullptr;

    this->runAction(
        CCSequence::create(
            CCDelayTime::create(0.5f),
            CCCallFunc::create(this, callfunc_selector(GlobedSignupPopup::onDelayedChallengeCompleted)),
            nullptr
        )
    );
}

void GlobedSignupPopup::uploadMessageFailed(int e) {
    GameLevelManager::sharedState()->m_uploadMessageDelegate = nullptr;
    this->onFailure(fmt::format("Message upload failed due to an unknown reason. Please try to open account settings and refresh login."));
}

void GlobedSignupPopup::onDelayedChallengeCompleted() {
    this->onChallengeCompleted(storedAuthcode);
}

void GlobedSignupPopup::onChallengeCompleted(const std::string_view authcode) {
    auto& csm = CentralServerManager::get();
    auto& am = GlobedAccountManager::get();

    statusMessage->setString("Verifying..");

    auto request = WebRequestManager::get().challengeFinish(authcode);
    finishListener.bind(this, &GlobedSignupPopup::finishCallback);
    finishListener.setFilter(std::move(request));
}

void GlobedSignupPopup::finishCallback(typename WebRequestManager::Event* event) {
    if (!event || !event->getValue()) return;

    auto evalue = std::move(*event->getValue());

    if (!evalue.ok()) {
        if (evalue.getCode() == 401) {
            this->onFailure(fmt::format("Account verification failure. Please try to refresh login in account settings.\n\nReason: <cy>{}</c>", evalue.text().unwrapOrDefault()));
            return;
        }

        this->onFailure(evalue.getError());
        return;
    }

    auto response = evalue.text().unwrapOrDefault();

    // we are good! the authkey has been created and can be saved now.
    auto colonPos = response.find(':');
    auto messageId = response.substr(0, colonPos);

    if (colonPos == std::string::npos) {
        log::warn("invalid challenge finish response: {}", response);
        this->onFailure("Completing challenge failed: <cy>invalid response from the server (no ':' found)</c>");
        return;
    }

    auto encodedAuthkey = response.substr(colonPos + 1);

    log::info("Authkey created successfully, saving.");
    util::data::bytevector authkey;
    try {
        authkey = util::crypto::base64Decode(encodedAuthkey);
    } catch (const std::exception& e) {
        log::warn("failed to decode authkey: {}", e.what());
        log::warn("authkey: {}", encodedAuthkey);
        this->onFailure("Completing challenge failed: <cy>invalid response from the server (couldn't decode base64)</c>");
        return;
    }

    auto& am = GlobedAccountManager::get();

    am.storeAuthKey(util::crypto::simpleHash(authkey));
    this->onSuccess();

    // delete the comment to cleanup
    if (messageId != "none") {
        auto message = GJUserMessage::create();
        message->m_messageID = util::format::parse<int>(messageId).value_or(-1);
        if (message->m_messageID == -1) return;

        GameLevelManager::sharedState()->deleteUserMessages(message, nullptr, true);
    }
}

void GlobedSignupPopup::onSuccess() {
    this->onClose(this);
}

void GlobedSignupPopup::onFailure(const std::string_view message) {
    ErrorQueues::get().error(message);
    this->onClose(this);
}

void GlobedSignupPopup::keyDown(cocos2d::enumKeyCodes) {
    // do nothing; the popup should be impossible to close manually
}

void GlobedSignupPopup::keyBackClicked() {
    // do nothing
}

GlobedSignupPopup* GlobedSignupPopup::create() {
    auto ret = new GlobedSignupPopup;
    if (ret->init(POPUP_WIDTH, POPUP_HEIGHT)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}