#include "signup_popup.hpp"

#include <UIBuilder.hpp>

#include <net/http/client.hpp>
#include <managers/error_queues.hpp>
#include <managers/central_server.hpp>
#include <managers/game_server.hpp>
#include <managers/account.hpp>
#include <util/net.hpp>
#include <util/formatting.hpp>
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

    auto url = activeServer->url + "/challenge/new?aid=" + std::to_string(am.gdData.lock()->accountId);

    GHTTPRequest::post(url)
        .userAgent(util::net::webUserAgent())
        .timeout(util::time::secs(5))
        .then([this](const GHTTPResponse& resp) {
            if (resp.anyfail()) {
                auto error = resp.anyfailmsg();
                if (error.empty()) {
                    this->onFailure("Creating challenge failed: server sent an empty response.");
                } else {
                    this->onFailure("Creating challenge failed: <cy>" + util::formatting::formatErrorMessage(error) + "</c>");
                }
            } else {
                auto response = resp.response;
                auto colonPos = response.find(':');
                auto part1 = response.substr(0, colonPos);
                auto part2 = response.substr(colonPos + 1);

                int levelId;

                if (part1 == "none") {
                    levelId = -1;
                } else {
                    levelId = std::stoi(part1);
                }

                this->onChallengeCreated(levelId, part2);
            }
        })
        .send();

    return true;
}

void GlobedSignupPopup::onChallengeCreated(int levelId, const std::string& chtoken) {
    // what are you cooking 😟
    // - xTymon 12.11.2023 19:03 CET

    auto hash = util::crypto::simpleHash(chtoken);
    auto authcode = util::crypto::simpleTOTP(hash);
    // TODO mac no worky yet, will have to find the symbols later

#ifndef GLOBED_MAC
    if (levelId == -1) {
#endif // GLOBED_MAC
        // skip posting the comment, server has it disabled
        this->onChallengeCompleted(authcode);
#ifndef GLOBED_MAC
    } else {
        statusMessage->setString("Uploading results..");
        storedAuthcode = authcode;
        storedLevelId = levelId;
        GameLevelManager::get()->m_commentUploadDelegate = this;
        GameLevelManager::get()->uploadLevelComment(levelId, authcode + " ## globed verification test, if you see this you can manually delete the comment.", 0);
    }
#endif // GLOBED_MAC
}

void GlobedSignupPopup::commentUploadFinished(int) {
    GameLevelManager::get()->m_commentUploadDelegate = nullptr;

    this->runAction(
        CCSequence::create(
            CCDelayTime::create(0.5f),
            CCCallFunc::create(this, callfunc_selector(GlobedSignupPopup::onDelayedChallengeCompleted)),
            nullptr
        )
    );
}

void GlobedSignupPopup::onDelayedChallengeCompleted() {
    this->onChallengeCompleted(storedAuthcode);
}

void GlobedSignupPopup::commentUploadFailed(int, CommentError e) {
    GameLevelManager::get()->m_commentUploadDelegate = nullptr;
    this->onFailure(fmt::format("Comment upload failed: <cy>error {}</c>", (int)e));
}

void GlobedSignupPopup::commentDeleteFailed(int, int) {}

void GlobedSignupPopup::onChallengeCompleted(const std::string& authcode) {
    auto& csm = CentralServerManager::get();
    auto& am = GlobedAccountManager::get();

    statusMessage->setString("Verifying..");

    auto gdData = am.gdData.lock();

    auto url = csm.getActive()->url +
        fmt::format("/challenge/verify?aid={}&aname={}&answer={}",
                    gdData->accountId,
                    gdData->accountName,
                    authcode);

    GHTTPRequest::post(url)
        .userAgent(util::net::webUserAgent())
        .timeout(util::time::secs(5))
        .then([this, &am](const GHTTPResponse& resp) {
            if (resp.anyfail()) {
                auto error = resp.anyfailmsg();
                if (error.empty()) {
                    this->onFailure("Creating challenge failed: server sent an empty response.");
                } else {
                    this->onFailure("Creating challenge failed: <cy>" + util::formatting::formatErrorMessage(error) + "</c>");
                }
            } else {
                auto response = resp.response;
                // we are good! the authkey has been created and can be saved now.
                auto colonPos = response.find(':');
                auto commentId = response.substr(0, colonPos);

                log::info("Authkey created successfully, saving.");
                auto authkey = util::crypto::base64Decode(response.substr(colonPos + 1));
                am.storeAuthKey(util::crypto::simpleHash(authkey));
                this->onSuccess();

#ifndef GLOBED_MAC
                // delete the comment to cleanup
                if (commentId != "none") {
                    GameLevelManager::get()->deleteComment(std::stoi(commentId), CommentType::Level, storedLevelId);
                }
#endif // GLOBED_MAC
            }
        })
        .send();
}

void GlobedSignupPopup::onSuccess() {
    this->onClose(this);
}

void GlobedSignupPopup::onFailure(const std::string& message) {
    ErrorQueues::get().error(message);
    this->onClose(this);
}

void GlobedSignupPopup::keyDown(cocos2d::enumKeyCodes) {
    // do nothing; the popup should be impossible to close manually
}
void GlobedSignupPopup::keyBackClicked() {
    // do nothing
};

GlobedSignupPopup* GlobedSignupPopup::create() {
    auto ret = new GlobedSignupPopup;
    if (ret->init(POPUP_WIDTH, POPUP_HEIGHT)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}