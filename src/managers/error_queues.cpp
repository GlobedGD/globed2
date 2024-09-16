#include "error_queues.hpp"

#include <defs/minimal_geode.hpp>

void ErrorQueues::warn(std::string_view message, bool print) {
    if (print) log::warn("{}", message);
    _warns.push(std::string(message));
}

void ErrorQueues::error(std::string_view message, bool print) {
    if (print) log::error("{}", message);
    _errors.push(std::string(message));
}

void ErrorQueues::success(std::string_view message, bool print) {
    if (print) log::info("{}", message);
    _successes.push(std::string(message));
}

void ErrorQueues::notice(std::string_view message, bool print) {
    if (print) log::warn("[Server notice] {}", message);
    _notices.push(std::string(message));
}

void ErrorQueues::debugWarn(std::string_view message, bool print) {
    if (print) log::warn("{}", message);
#if defined(GLOBED_DEBUG) && GLOBED_DEBUG
    _warns.push(std::string(message));
#endif
}

std::vector<std::string> ErrorQueues::getWarnings() {
    std::vector<std::string> out;
    while (auto msg = _warns.tryPop()) {
        out.push_back(msg.value());
    }

    return out;
}

std::vector<std::string> ErrorQueues::getErrors() {
    std::vector<std::string> out;
    while (auto msg = _errors.tryPop()) {
        out.push_back(msg.value());
    }

    return out;
}

std::vector<std::string> ErrorQueues::getSuccesses() {
    std::vector<std::string> out;
    while (auto msg = _successes.tryPop()) {
        out.push_back(msg.value());
    }

    return out;
}

std::vector<std::string> ErrorQueues::getNotices() {
    std::vector<std::string> out;
    while (auto msg = _notices.tryPop()) {
        out.push_back(msg.value());
    }

    return out;
}