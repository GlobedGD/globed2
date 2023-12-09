#include "error_queues.hpp"

ErrorQueues::ErrorQueues() {}

void ErrorQueues::warn(const std::string& message, bool print) {
    if (print) geode::log::warn(message);
    _warns.push(message);
}

void ErrorQueues::error(const std::string& message, bool print) {
    if (print) geode::log::error(message);
    _errors.push(message);
}

void ErrorQueues::notice(const std::string& message, bool print) {
    if (print) geode::log::warn("[Server notice] {}", message);
    _notices.push(message);
}

void ErrorQueues::debugWarn(const std::string& message, bool print) {
    if (print) geode::log::warn(message);
#if defined(GLOBED_DEBUG) && GLOBED_DEBUG
    _warns.push(message);
#endif
}

std::vector<std::string> ErrorQueues::getWarnings() {
    return _warns.popAll();
}

std::vector<std::string> ErrorQueues::getErrors() {
    return _errors.popAll();
}

std::vector<std::string> ErrorQueues::getNotices() {
    return _notices.popAll();
}