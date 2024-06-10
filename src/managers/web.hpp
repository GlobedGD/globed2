#pragma once

#include <defs/geode.hpp>
#include <optional>
#include <functional>
#include <Geode/utils/web.hpp>

#include <util/singleton.hpp>

struct WebRequestError {
    int code;
    std::string message;

    WebRequestError(int code, const std::string& message) : code(code), message(message) {}
    WebRequestError() : code(0), message({}) {}
};

class WebRequestManager : public SingletonBase<WebRequestManager> {
    friend class SingletonBase;

public:
    using RequestTask = geode::Task<geode::Result<std::string, WebRequestError>>;
    using RequestListener = geode::EventListener<RequestTask>;
    using SingletonBase::get;

    RequestTask requestAuthToken();
    RequestTask testServer(std::string_view url);
    RequestTask fetchCredits();
    RequestTask fetchServers();
    RequestTask challengeStart();
    RequestTask challengeFinish(std::string_view authcode);

private:

    RequestTask get(std::string_view url);
    RequestTask get(std::string_view url, int timeoutS);
    RequestTask get(std::string_view url, int timeoutS, std::function<void(geode::utils::web::WebRequest&)> additional);

    RequestTask post(std::string_view url);
    RequestTask post(std::string_view url, int timeoutS);
    RequestTask post(std::string_view url, int timeoutS, std::function<void(geode::utils::web::WebRequest&)> additional);

};
