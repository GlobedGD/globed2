#include "sync.hpp"

namespace util::sync {

GLOBED_SINGLETON_GET(ErrorQueues)
ErrorQueues::ErrorQueues() {}

void ErrorQueues::warn(const std::string& message) {
    _warns.push(message);
}

void ErrorQueues::error(const std::string& message) {
    _errors.push(message);
}

std::vector<std::string> ErrorQueues::getWarnings() {
    return _warns.popAll();
}

std::vector<std::string> ErrorQueues::getErrors() {
    return _errors.popAll();
}

}