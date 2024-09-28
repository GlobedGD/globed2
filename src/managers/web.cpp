#include "web.hpp"

#include <managers/account.hpp>
#include <managers/central_server.hpp>
#include <util/net.hpp>
#include <util/time.hpp>
#include <net/manager.hpp>

using namespace geode::prelude;

using RequestTask = WebRequestManager::Task;

static std::string makeUrl(std::string_view baseUrl, std::string_view suffix) {
    std::string base(baseUrl);

    if (suffix.starts_with('/') && base.ends_with('/')) {
        base.pop_back();
    } else if (!suffix.starts_with('/') && !base.ends_with('/')) {
        base.push_back('/');
    }

    base += suffix;

    return base;
}

static std::string makeCentralUrl(std::string_view suffix) {
    auto& csm = CentralServerManager::get();
    auto active = csm.getActive();

    GLOBED_REQUIRE(active, "trying to make a web request to a central server when no active central server is selected");

    return makeUrl(active->url, suffix);
}

static RequestTask mapTask(CurlManager::Task&& param) {
    return param;
}

RequestTask WebRequestManager::requestAuthToken() {
    auto& gam = GlobedAccountManager::get();

    auto authkey = gam.getAuthKey().value_or("");
    auto gdData = gam.gdData.lock();

    return this->post(makeCentralUrl("v2/totplogin"), 10, [&](CurlRequest& req) {
        matjson::Object accdata;
        accdata["account_id"] = gdData->accountId;
        accdata["user_id"] = gdData->userId;
        accdata["username"] = gdData->accountName;

        matjson::Object obj;
        obj["account_data"] = accdata;

        // recode as urlsafe
        obj["authkey"] = authkey;

        req.bodyJSON(obj);
        req.encrypted(true);
        req.param("protocol", NetworkManager::get().getUsedProtocol());
    });
}

RequestTask WebRequestManager::challengeStart() {
    auto& gam = GlobedAccountManager::get();

    auto gdData = gam.gdData.lock();

    return this->post(makeCentralUrl("v2/challenge/new"), 10, [&](CurlRequest& req) {
        matjson::Object accdata;
        accdata["account_id"] = gdData->accountId;
        accdata["user_id"] = gdData->userId;
        accdata["username"] = gdData->accountName;

        req.bodyJSON(accdata);
        req.encrypted(true);
        req.param("protocol", NetworkManager::get().getUsedProtocol());
    });
}

RequestTask WebRequestManager::challengeFinish(std::string_view authcode) {
    auto& gam = GlobedAccountManager::get();

    auto gdData = gam.gdData.lock();

    return this->post(makeCentralUrl("v2/challenge/verify"), 30, [&](CurlRequest& req) {
        matjson::Object accdata;
        accdata["account_id"] = gdData->accountId;
        accdata["user_id"] = gdData->userId;
        accdata["username"] = gdData->accountName;

        matjson::Object obj;
        obj["account_data"] = accdata;
        obj["answer"] = std::string(authcode);

        req.bodyJSON(obj);
        req.encrypted(true);
    });
}

RequestTask WebRequestManager::testServer(std::string_view url) {
    return this->get(makeUrl(url, "versioncheck"), 10, [&](CurlRequest& req) {
        // why do you have to do this to me geode.

        // TODO: uncomment this pls when geode 3.7.2 or later
        // req.param("gd", GEODE_GD_VERSION_STRING);
        req.param("gd", "2.206");
        req.param("globed", Mod::get()->getVersion().toNonVString());
        req.param("protocol", NetworkManager::get().getUsedProtocol());
    });
}

RequestTask WebRequestManager::fetchCredits() {
    return this->get("https://credits.globed.dev/credits");
}

RequestTask WebRequestManager::fetchServers() {
    return this->get(makeCentralUrl("servers"), 10, [&](CurlRequest& req) {
        req.param("protocol", NetworkManager::get().getUsedProtocol());
    });
}

RequestTask WebRequestManager::fetchFeaturedLevel() {
    return this->get(makeCentralUrl("flevel/current"));
}

RequestTask WebRequestManager::fetchFeaturedLevelHistory(int page) {
    return this->get(makeCentralUrl("flevel/historyv2"), 10, [&](CurlRequest& req) {
        req.param("page", page);
    });
}

RequestTask WebRequestManager::setFeaturedLevel(int levelId, int rateTier, std::string_view levelName, std::string_view levelAuthor, int difficulty) {
    return this->post(makeCentralUrl("v2/flevel/replace"), 10, [&](CurlRequest& req) {
        matjson::Object data = {
            {"level_id", levelId},
            {"rate_tier", rateTier},
            {"account_id", GlobedAccountManager::get().gdData.lock()->accountId},
            {"admin_password", GlobedAccountManager::get().getTempAdminPassword()},
            {"level_name", std::string(levelName)},
            {"level_author", std::string(levelAuthor)},
            {"difficulty", difficulty}
        };

        req.bodyJSON(data);
        req.encrypted(true);
    });
}

RequestTask WebRequestManager::get(std::string_view url) {
    return get(url, 10);
}

RequestTask WebRequestManager::get(std::string_view url, int timeoutS) {
    return get(url, timeoutS, [](auto&) {});
}

RequestTask WebRequestManager::get(std::string_view url, int timeoutS, std::function<void(CurlRequest&)> additional) {
#ifdef GLOBED_DEBUG
    log::debug("GET request: {}", url);
#endif

    auto request = CurlRequest()
        .timeout(util::time::seconds(timeoutS));

    additional(request);

    return mapTask(request.get(url).send());
}


RequestTask WebRequestManager::post(std::string_view url) {
    return post(url, 10);
}

RequestTask WebRequestManager::post(std::string_view url, int timeoutS) {
    return post(url, timeoutS, [](auto&) {});
}

RequestTask WebRequestManager::post(std::string_view url, int timeoutS, std::function<void(CurlRequest&)> additional) {
#ifdef GLOBED_DEBUG
    log::debug("POST request: {}", url);
#endif

    auto request = CurlRequest()
        .timeout(util::time::seconds(timeoutS));

    additional(request);

    return mapTask(request.post(url).send());
}

