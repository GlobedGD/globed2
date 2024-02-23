#include "signup_popup.hpp"

#include <Geode/utils/web.hpp>
#include <managers/error_queues.hpp>
#include <managers/central_server.hpp>
#include <managers/game_server.hpp>
#include <managers/account.hpp>
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

    auto gdData = am.gdData.lock();
    auto url = fmt::format(
        "{}/challenge/new?aid={}&uid={}&aname={}",
        activeServer->url,
        gdData->accountId,
        gdData->userId,
        util::format::urlEncode(gdData->accountName)
    );

    web::AsyncWebRequest()
        .userAgent(util::net::webUserAgent())
        .timeout(util::time::seconds(5))
        .post(url)
        .text()
        .then([this](std::string& response) {
            auto colonPos = response.find(':');
            auto part1 = response.substr(0, colonPos);
            auto part2 = response.substr(colonPos + 1);

            // we accept -1 as the default ()
            int accountId = util::format::parse<int>(part1).value_or(-1);

            this->onChallengeCreated(accountId, part2);
        })
        .expect([this](std::string error, int statusCode) {
            if (error.empty()) {
                this->onFailure(fmt::format("Creating challenge failed: server sent an empty response with code {}.", statusCode));
            } else {
                this->onFailure("Creating challenge failed: <cy>" + util::format::formatErrorMessage(error) + "</c>");
            }
        })
        .send();

    return true;
}

void GlobedSignupPopup::onChallengeCreated(int accountId, const std::string_view chtoken) {
    auto hash = util::crypto::simpleHash(chtoken);
    auto authcode = util::crypto::simpleTOTP(hash);

    if (accountId == -1) {
        // skip the account verification, server has it disabled
        this->onChallengeCompleted(authcode);
    } else {
        statusMessage->setString("Uploading results..");
        storedAuthcode = authcode;
        storedAccountId = accountId;
        GameLevelManager::sharedState()->m_uploadMessageDelegate = this;
        GameLevelManager::sharedState()->uploadUserMessage(storedAccountId, fmt::format("##c## {}", authcode), "hi this is a globed verification test, please delete this message if you see it.");
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

    auto gdData = am.gdData.lock();

    auto url = csm.getActive()->url +
        fmt::format("/challenge/verify?aid={}&uid={}&aname={}&answer={}&systime={}",
                    gdData->accountId,
                    gdData->userId,
                    util::format::urlEncode(gdData->accountName),
                    authcode,
                    std::time(nullptr)
        );

    web::AsyncWebRequest()
        .userAgent(util::net::webUserAgent())
        .timeout(util::time::seconds(10))
        .post(url)
        .text()
        .then([this, &am](std::string& response) {
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

            am.storeAuthKey(util::crypto::simpleHash(authkey));
            this->onSuccess();

            // delete the comment to cleanup
            if (messageId != "none") {
                auto message = GJUserMessage::create();
                message->m_messageID = util::format::parse<int>(messageId).value_or(-1);
                if (message->m_messageID == -1) return;

                GameLevelManager::sharedState()->deleteUserMessages(message, nullptr, true);
            }
        })
        .expect([this](std::string error, int statusCode) {
            if (error.empty()) {
                this->onFailure(fmt::format("Completing challenge failed: server sent an empty response with code {}.", statusCode));
            } else if (statusCode == 401) {
                this->onFailure("Completing challenge failed: account verification failure. Please try to refresh login in account settings.\n\nReason: <cy>" + util::format::formatErrorMessage(error) + "</c>");
            } else {
                this->onFailure("Completing challenge failed: <cy>" + util::format::formatErrorMessage(error) + "</c>");
            }
        })
        .send();
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