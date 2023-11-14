#pragma once
#include <defs.hpp>
#include <vector>
#include <string>
#include <util/sync.hpp>

/*
* ErrorQueues is a thread safe singleton for propagating errors to the main thread,
* so they can be shown to the end user.
*/

class ErrorQueues {
    GLOBED_SINGLETON(ErrorQueues);
    ErrorQueues();

    void warn(const std::string& message, bool print = true);
    void error(const std::string& message, bool print = true);

    std::vector<std::string> getWarnings();
    std::vector<std::string> getErrors();
private:
    util::sync::SmartMessageQueue<std::string> _warns, _errors;
};