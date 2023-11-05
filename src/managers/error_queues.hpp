#pragma once
#include <defs.hpp>
#include <vector>
#include <string>
#include <util/sync.hpp>


/*
* ErrorQueues is a simple singleton that has 2 smart message queues for errors and warnings
* made to send network errors over to main thread to display them
*/
class ErrorQueues {
public:
    GLOBED_SINGLETON(ErrorQueues);
    ErrorQueues();

    void warn(const std::string& message);
    void error(const std::string& message);

    std::vector<std::string> getWarnings();
    std::vector<std::string> getErrors();
private:
    util::sync::SmartMessageQueue<std::string> _warns, _errors;
};