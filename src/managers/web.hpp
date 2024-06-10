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
    using Result = geode::Result<std::string, WebRequestError>;
    using Task = geode::Task<Result>;
    using Listener = geode::EventListener<Task>;
    using Event = Task::Event;
    using SingletonBase::get;

    Task requestAuthToken();
    Task testServer(std::string_view url);
    Task fetchCredits();
    Task fetchServers();
    Task challengeStart();
    Task challengeFinish(std::string_view authcode);

private:

    Task get(std::string_view url);
    Task get(std::string_view url, int timeoutS);
    Task get(std::string_view url, int timeoutS, std::function<void(geode::utils::web::WebRequest&)> additional);

    Task post(std::string_view url);
    Task post(std::string_view url, int timeoutS);
    Task post(std::string_view url, int timeoutS, std::function<void(geode::utils::web::WebRequest&)> additional);

};
