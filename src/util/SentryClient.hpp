#pragma once
#include <globed/util/singleton.hpp>
#include <arc/prelude.hpp>

namespace globed {

enum class SentryIssueLevel {
    Fatal,
    Error,
    Warning,
    Info,
    Debug,
};

struct SentryIssueReport {
    SentryIssueLevel level;
    std::unordered_map<std::string, std::string> tags;
    std::string message;
    std::optional<int> userId;
};

class SentryClient : public SingletonBase<SentryClient> {
public:
    SentryClient();

    /// Generic issue report, most customizable.
    arc::Future<> reportIssue(SentryIssueReport report);

    // Specialized functions for specific issues

    arc::Future<> reportArgonIssue(int accountId, std::string error);
    arc::Future<> reportCentralConnectionError(std::string error);
    arc::Future<> reportGameConnectionError(std::string url, std::string error);

private:
    std::string m_url, m_key, m_env;
    bool m_enabled = false;
};

}
