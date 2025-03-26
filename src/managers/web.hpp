#pragma once

#include <defs/geode.hpp>
#include <functional>

#include <util/singleton.hpp>
#include <managers/curl.hpp>

class WebRequestManager : public SingletonBase<WebRequestManager> {
    friend class SingletonBase;

public:
    using Task = CurlManager::Task;
    using Listener = geode::EventListener<Task>;
    using Event = Task::Event;
    using SingletonBase::get;

    Task requestAuthToken();
    Task challengeStart();
    Task challengeFinish(std::string_view authcode, const std::string&);

    Task testServer(std::string_view url);
    Task fetchCredits();
    Task fetchServers(std::string_view urlOverride = {});
    Task fetchFeaturedLevel();
    Task fetchFeaturedLevelHistory(int page);
    Task setFeaturedLevel(int levelId, int rateTier, std::string_view levelName, std::string_view levelAuthor, int difficulty);

    // Makes a HEAD request to https://google.com
    Task testGoogle();

private:
    Task get(std::string_view url);
    Task get(std::string_view url, int timeoutS);
    Task get(std::string_view url, int timeoutS, std::function<void(CurlRequest&)> additional);

    Task post(std::string_view url);
    Task post(std::string_view url, int timeoutS);
    Task post(std::string_view url, int timeoutS, std::function<void(CurlRequest&)> additional);

    Task head(std::string_view url, int timeoutS = 5);
};
