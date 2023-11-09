#include "error_queues.hpp"

ErrorQueues::ErrorQueues() {}
GLOBED_SINGLETON_DEF(ErrorQueues)

void ErrorQueues::warn(const std::string& message, bool print) {
    if (print) geode::log::warn(message);
    _warns.push(message);
}

void ErrorQueues::error(const std::string& message, bool print) {
    if (print) geode::log::error(message);
    _errors.push(message);
}

std::vector<std::string> ErrorQueues::getWarnings() {
    return _warns.popAll();
}

std::vector<std::string> ErrorQueues::getErrors() {
    return _errors.popAll();
}