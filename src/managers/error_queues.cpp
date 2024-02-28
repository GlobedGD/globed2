#include "error_queues.hpp"

#include <defs/minimal_geode.hpp>

void ErrorQueues::warn(const std::string_view message, bool print) {
    if (print) log::warn("{}", message);
    _warns.push(std::string(message));
}

void ErrorQueues::error(const std::string_view message, bool print) {
    if (print) log::error("{}", message);
    _errors.push(std::string(message));
}

void ErrorQueues::success(const std::string_view message, bool print) {
    if (print) log::info("{}", message);
    _successes.push(std::string(message));
}

void ErrorQueues::notice(const std::string_view message, bool print) {
    if (print) log::warn("[Server notice] {}", message);
    _notices.push(std::string(message));
}

void ErrorQueues::debugWarn(const std::string_view message, bool print) {
    if (print) log::warn("{}", message);
#if defined(GLOBED_DEBUG) && GLOBED_DEBUG
    _warns.push(std::string(message));
#endif
}

std::vector<std::string> ErrorQueues::getWarnings() {
    return _warns.popAll();
}

std::vector<std::string> ErrorQueues::getErrors() {
    return _errors.popAll();
}

std::vector<std::string> ErrorQueues::getSuccesses() {
    return _successes.popAll();
}

std::vector<std::string> ErrorQueues::getNotices() {
    return _notices.popAll();
}