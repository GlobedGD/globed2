#include "SentryClient.hpp"
#include <globed/core/Constants.hpp>
#include <globed/core/ServerManager.hpp>
#include <glaze/json.hpp>

using namespace geode::prelude;
using namespace arc;

namespace globed {

struct EnvelopeUser {
    std::string id;
};

struct EnvelopePayload {
    std::string platform;
    std::vector<std::string> errors;
    std::unordered_map<std::string, std::string> tags;
    std::string event_id;
    std::string timestamp;
    std::string level;
    std::string message;
    std::optional<EnvelopeUser> user;
};

inline auto format_as(SentryIssueLevel level) -> std::string {
    switch (level) {
        case SentryIssueLevel::Fatal: return "fatal";
        case SentryIssueLevel::Error: return "error";
        case SentryIssueLevel::Warning: return "warning";
        case SentryIssueLevel::Info: return "info";
        case SentryIssueLevel::Debug: return "debug";
    }
    return "unknown";
}

SentryClient::SentryClient() {
    m_url = globed::constant<"sentry-url">();
    m_key = globed::constant<"sentry-key">();
    m_env = globed::constant<"sentry-env">();

    if (m_url.empty() || m_key.empty()) {
        m_enabled = false;
    } else {
        log::debug("Using sentry URL {}, environment '{}'", m_url, m_env);
        m_enabled = true;
    }
}

Future<> SentryClient::reportIssue(SentryIssueReport report) {
    if (!m_enabled) {
        log::trace("Skipping sentry event due to client being disabled");
        co_return;
    }

    EnvelopePayload payload{};
    payload.event_id = utils::random::generateUUID();
    payload.platform = utils::platform::getString();
    payload.tags = std::move(report.tags);
    payload.timestamp = asp::SystemTime::now().format("{:%Y-%m-%dT%H:%M:%S}.{:03}Z");
    payload.level = fmt::to_string(report.level);
    payload.message = std::move(report.message);

    if (report.userId) {
        payload.user = EnvelopeUser{fmt::to_string(*report.userId)};
    }

    auto resp = co_await web::WebRequest{}
        .header("Content-Type", "application/json")
        .header("X-Sentry-Auth", fmt::format(
            "Sentry sentry_version=7, sentry_client=globed/{}, sentry_key={}",
            Mod::get()->getVersion().toNonVString(), m_key
        ))
        .bodyJSON(glz::write_json(payload).value_or(""))
        .post(m_url);

    if (!resp.ok()) {
        log::warn("Failed to send sentry event (code {}): {}", resp.code(), resp.string().unwrapOrDefault());
        co_return;
    }

    log::debug("Sentry event sent successfully (code {})", resp.code());
}

Future<> SentryClient::reportArgonIssue(int accountId, std::string error) {
    return this->reportIssue(SentryIssueReport {
        .level = SentryIssueLevel::Error,
        .tags = {{"component", "argon"}, {"kind", "auth"}},
        .message = std::move(error),
        .userId = accountId
    });
}

Future<> SentryClient::reportCentralConnectionError(std::string error) {
    bool isMainServer = ServerManager::get().isOfficialServerActive();
    if (!isMainServer) {
        // ignore custom servers
        co_return;
    }

    co_await this->reportIssue(SentryIssueReport {
        .level = SentryIssueLevel::Error,
        .tags = {{"component", "central"}, {"kind", "connection"}},
        .message = std::move(error),
        .userId = std::nullopt
    });
}

Future<> SentryClient::reportGameConnectionError(std::string url, std::string error) {
    bool isMainServer = ServerManager::get().isOfficialServerActive();
    if (!isMainServer) {
        // ignore custom servers
        co_return;
    }

    co_await this->reportIssue(SentryIssueReport {
        .level = SentryIssueLevel::Error,
        .tags = {{"component", "game"}, {"kind", "connection"}, {"url", url}},
        .message = std::move(error),
        .userId = std::nullopt
    });
}

}
