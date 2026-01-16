// Sourced from
// https://github.com/Kingminer7/server-api/blob/main/include/ServerAPIEvents.hpp
// Commit aec29ad

#pragma once

using namespace geode::prelude;

#ifdef GEODE_IS_WINDOWS
#ifdef KM7DEV_SERVER_API_EXPORTING
#define SERVER_API_DLL __declspec(dllexport)
#else
#define SERVER_API_DLL __declspec(dllimport)
#endif
#else
#define SERVER_API_DLL __attribute__((visibility("default")))
#endif

struct GetCurrentServerEvent : geode::Event {
    GetCurrentServerEvent() {}

    int m_id = 0;
    std::string m_url = "";
    int m_prio = 0;
};

struct GetServerByIdEvent : geode::Event {
    GetServerByIdEvent(int id) : m_id(std::move(id)) {}

    int m_id = 0;
    std::string m_url = "";
    int m_prio = 0;
};

struct RegisterServerEvent : geode::Event {
    RegisterServerEvent(std::string url, int priority) : m_url(std::move(url)), m_priority(std::move(priority)) {}

    int m_id;
    std::string m_url = "";
    int m_priority = 0;
};

struct UpdateServerEvent : geode::Event {
    UpdateServerEvent(int id, std::string url, int priority)
        : m_id(std::move(id)), m_url(std::move(url)), m_priority(std::move(priority))
    {
    }
    UpdateServerEvent(int id, std::string url) : m_id(std::move(id)), m_url(std::move(url)) {}
    UpdateServerEvent(int id, int priority) : m_id(std::move(id)), m_priority(std::move(priority)) {}

    int m_id = 0;
    std::string m_url = "";
    int m_priority = 0;
};

struct RemoveServerEvent : geode::Event {
    RemoveServerEvent(int id) : m_id(std::move(id)) {}

    int m_id = 0;
};

struct GetBaseUrlEvent : geode::Event {
    GetBaseUrlEvent() {}

    std::string m_url = "";
};

struct GetSecondaryUrlEvent : geode::Event {
    GetSecondaryUrlEvent() {}

    std::string m_url = "";
};

struct GetRegisteredServersEvent : geode::Event {
    GetRegisteredServersEvent() {}

    std::map<int, std::pair<std::string, int>> m_servers = {};
};

// thx prevter for the help

namespace ServerAPIEvents {

struct Server {
    int id;
    std::string url;
    int priority;
};

inline Server getCurrentServer()
{
    auto event = GetCurrentServerEvent();
    event.post();
    return {event.m_id, event.m_url, event.m_prio};
}

inline Server getServerById(int id)
{
    auto event = GetServerByIdEvent(id);
    event.post();
    return {event.m_id, event.m_url, event.m_prio};
}

inline Server registerServer(std::string url, int priority = 0)
{
    auto event = RegisterServerEvent(url, priority);
    event.post();
    return {event.m_id, event.m_url, event.m_priority};
}

inline void updateServer(Server server)
{
    auto event = UpdateServerEvent(server.id, server.url, server.priority);
    event.post();
}

inline void updateServer(int id, std::string url, int priority)
{
    auto event = UpdateServerEvent(id, url, priority);
    event.post();
}

inline void updateServer(int id, std::string url)
{
    auto event = UpdateServerEvent(id, url);
    event.post();
}

inline void updateServer(int id, int priority)
{
    auto event = UpdateServerEvent(id, priority);
    event.post();
}

inline void removeServer(int id)
{
    auto event = RemoveServerEvent(id);
    event.post();
}

inline std::string getBaseUrl()
{
    auto event = GetBaseUrlEvent();
    event.post();
    return event.m_url;
}

inline std::string getSecondaryUrl()
{
    auto event = GetSecondaryUrlEvent();
    event.post();
    return event.m_url;
}

inline std::map<int, std::pair<std::string, int>> getRegisteredServers()
{
    auto event = GetRegisteredServersEvent();
    event.post();
    return event.m_servers;
}
} // namespace ServerAPIEvents
