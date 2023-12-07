#pragma once
#include <defs.hpp>

#include <vector>

#include <util/sync.hpp>

/*
* ErrorQueues is a thread safe singleton for propagating errors to the main thread,
* so they can be shown to the end user.
*/

class ErrorQueues {
    GLOBED_SINGLETON(ErrorQueues)
    ErrorQueues();

    void warn(const std::string& message, bool print = true);
    void error(const std::string& message, bool print = true);
    // notices are messages coming directly from the server
    void notice(const std::string& message, bool print = true);

    // debugWarn shows a warn notification in debug, in release only prints a message (noop if `print` = false)
    void debugWarn(const std::string& message, bool print = true);

    std::vector<std::string> getWarnings();
    std::vector<std::string> getErrors();
    std::vector<std::string> getNotices();
private:
    util::sync::SmartMessageQueue<std::string> _warns, _errors, _notices;
};