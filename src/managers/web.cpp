#include "web.hpp"

#include <globed/tracing.hpp>
#include <managers/account.hpp>
#include <managers/central_server.hpp>
#include <managers/settings.hpp>
#include <util/net.hpp>
#include <net/manager.hpp>

#include <asp/time/SystemTime.hpp>
#include <matjson/reflect.hpp>

using namespace geode::prelude;
using namespace asp::time;

using RequestTask = WebRequestManager::Task;
using DohTask = WebRequestManager::DohTask;

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

RequestTask WebRequestManager::requestAuthToken(bool argon) {
    auto& gam = GlobedAccountManager::get();

    auto authkey = gam.getAuthKey().value_or("");
    auto argonToken = gam.getArgonToken();

    auto gdData = gam.gdData.lock();

    return this->post(makeCentralUrl(argon ? "v3/argonlogin" : "v2/totplogin"), 10, [&](CurlRequest& req) {
        matjson::Value accdata = matjson::Value::object();
        accdata["account_id"] = gdData->accountId;
        accdata["user_id"] = gdData->userId;
        accdata["username"] = gdData->accountName;

        matjson::Value obj = matjson::Value::object();
        obj["account_data"] = accdata;

        if (!argon) {
            obj["authkey"] = authkey;
        } else {
            obj["token"] = argonToken;
            if (auto s = NetworkManager::get().getSecure(fmt::to_string(gdData->accountId))) {
                obj[GEODE_STR(GEODE_CONCAT(GEODE_CONCAT(tr, ust), GEODE_CONCAT(_tok, en)))] = s.value();
            }
        }

        req.bodyJSON(obj);
        req.encrypted(true);

        if (!argon) {
            req.param("protocol", NetworkManager::get().getUsedProtocol());
        }
    });
}

RequestTask WebRequestManager::challengeStart() {
    auto& gam = GlobedAccountManager::get();

    auto gdData = gam.gdData.lock();

    return this->post(makeCentralUrl("v2/challenge/new"), 10, [&](CurlRequest& req) {
        auto accdata = matjson::Value::object();
        accdata["account_id"] = gdData->accountId;
        accdata["user_id"] = gdData->userId;
        accdata["username"] = gdData->accountName;

        req.bodyJSON(accdata);
        req.encrypted(true);
        req.param("protocol", NetworkManager::get().getUsedProtocol());
    });
}

RequestTask WebRequestManager::challengeFinish(std::string_view authcode, const std::string& challenge) {
    auto& gam = GlobedAccountManager::get();

    auto gdData = gam.gdData.lock();

    return this->post(makeCentralUrl("v2/challenge/verify"), 30, [&](CurlRequest& req) {
        auto accdata = matjson::Value::object();
        accdata["account_id"] = gdData->accountId;
        accdata["user_id"] = gdData->userId;
        accdata["username"] = gdData->accountName;

        auto obj = matjson::Value::object();
        obj["account_data"] = accdata;
        obj["answer"] = std::string(authcode);

        if (!challenge.empty()) {
            if (auto s = NetworkManager::get().getSecure(challenge)) {
                obj[GEODE_STR(GEODE_CONCAT(GEODE_CONCAT(tr, ust), GEODE_CONCAT(_tok, en)))] = s.value();
            }
        }

        req.bodyJSON(obj);
        req.encrypted(true);
    });
}

RequestTask WebRequestManager::testServer(std::string_view url) {
    return this->get(makeUrl(url, "versioncheck"), 10, [&](CurlRequest& req) {
        req.param("gd", GEODE_GD_VERSION_STRING);
        req.param("globed", Mod::get()->getVersion().toNonVString());
        req.param("protocol", NetworkManager::get().getUsedProtocol());
    });
}

RequestTask WebRequestManager::fetchCredits() {
    return this->get("https://credits.globed.dev/credits");
}

RequestTask WebRequestManager::fetchServers(std::string_view urlOverride) {
    auto url = urlOverride.empty() ? makeCentralUrl("servers") : makeUrl(urlOverride, "servers");

    return this->get(url, 10, [&](CurlRequest& req) {
        req.param("protocol", NetworkManager::get().getUsedProtocol());
    });
}

RequestTask WebRequestManager::fetchServerMeta(std::string_view urlOverride) {
    auto url = urlOverride.empty() ? makeCentralUrl("v3/meta") : makeUrl(urlOverride, "v3/meta");

    return this->get(url, 10, [&](CurlRequest& req) {
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
        auto data = matjson::makeObject({
            {"level_id", levelId},
            {"rate_tier", rateTier},
            {"account_id", GlobedAccountManager::get().gdData.lock()->accountId},
            {"admin_password", GlobedAccountManager::get().getTempAdminPassword()},
            {"level_name", std::string(levelName)},
            {"level_author", std::string(levelAuthor)},
            {"difficulty", difficulty}
        });

        req.bodyJSON(data);
        req.encrypted(true);
    });
}

RequestTask WebRequestManager::testGoogle() {
    return this->head("https://google.com");
}

RequestTask WebRequestManager::testCloudflare() {
    return this->testCloudflareDomainTrace("www.cloudflare.com");
}

RequestTask WebRequestManager::testCloudflareDomainTrace(std::string_view domain) {
    return this->get(fmt::format("https://{}/cdn-cgi/trace", domain));
}

struct DohQuestion {
    std::string name;
    int type;
};

struct DohAnswer {
    std::string name;
    int type;
    int TTL;
    std::string data;
};

struct DohResponse {
    int Status;
    bool TC;
    bool RD;
    bool RA;
    bool AD;
    bool CD;
    std::vector<DohQuestion> Question;
    std::vector<DohAnswer> Answer;
};

DohTask WebRequestManager::dnsOverHttps(std::string_view hostname, std::string_view family) {
    globed::netLog("Performing DoH lookup for {} ({})", hostname, family);

    return this->get("https://one.one.one.one/dns-query", 5, [&](CurlRequest& req) {
        req.param("name", hostname);
        req.param("type", family);
        req.param("do", "false");
        req.param("cd", "false");
        req.header("accept", "application/dns-json");
    }).map([](CurlResponse* response) -> Result<std::string> {
        globed::netLog("Received DoH response (code {})", response->getCode());

        if (!response->ok()) {
            globed::netLog("(E) DoH lookup failed: {}", response->getError());
            return Err("DoH lookup failed: {}", response->getError());
        }

        auto data = matjson::parseAs<DohResponse>(response->text().unwrapOrDefault());
        if (!data) {
            globed::netLog("(E) DoH response is invalid: {}", response->text().unwrapOrDefault());
            log::warn("DoH response is invalid JSON: {}", response->text().unwrapOrDefault());
            return Err("Failed to parse DoH response as JSON: {}", data.unwrapErr());
        }

        int status = (*data).Status;
        auto& answers = (*data).Answer;
        auto& questions = (*data).Question;

        std::string asked = "<unknown>";
        if (!questions.empty()) {
            asked = questions[0].name;
        }

        if (status != 0) {
            globed::netLog("(E) DoH response returned DNS status code {} for {}", status, asked);
            return Err("DoH lookup failed with status code {} for {}", status, asked);
        }

        if (answers.empty()) {
            globed::netLog("(E) DoH response returned 0 answers for {}", asked);
            return Err("DoH lookup returned no answers for {}", asked);
        }

        auto& record = answers[0];
        globed::netLog("DoH lookup succeeded: {} -> {} (type {}, TTL {})", asked, record.data, record.type, record.TTL);

        return Ok(record.data);
    });
}

bool WebRequestManager::isRussian() {
    return m_russian.value_or(false);
}

void WebRequestManager::fetchCountry() {
    if (m_russian.has_value()) {
        return;
    }

    this->testCloudflareDomainTrace("boomlings.com").listen([this](CurlResponse* response) {
        if (!response) return;

        if (!response->ok()) {
            log::warn("Failed to fetch cdn-cgi trace (code {}): {}", response->getCode(), response->text().unwrapOrDefault());
            m_russian = true;
            return;
        }

        auto text = response->text().unwrapOrDefault();
        auto locpos = text.find("loc=") + 4;
        std::string_view loc = std::string_view{text}.substr(locpos, 2);

        m_russian = (loc == "RU" || loc == "BY"); // i dont think belarus blocks but im adding here to be safe
    });
}

RequestTask WebRequestManager::get(std::string_view url) {
    return get(url, 10);
}

RequestTask WebRequestManager::get(std::string_view url, int timeoutS) {
    return get(url, timeoutS, [](auto&) {});
}

RequestTask WebRequestManager::get(std::string_view url, int timeoutS, std::function<void(CurlRequest&)> additional) {
    TRACE("GET request: {}", url);

    auto request = CurlRequest()
        .timeout(Duration::fromSecs(timeoutS));

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
    TRACE("POST request: {}", url);

    auto request = CurlRequest()
        .timeout(Duration::fromSecs(timeoutS));

    additional(request);

    return mapTask(request.post(url).send());
}

RequestTask WebRequestManager::head(std::string_view url, int timeoutS) {
    TRACE("HEAD request: {}", url);

    auto request = CurlRequest()
        .timeout(Duration::fromSecs(timeoutS));

    return mapTask(request.head(url).send());
}
