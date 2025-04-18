#pragma once

#include <asp/sync.hpp>

#include <util/singleton.hpp>

/*
* ErrorQueues is a thread safe singleton for propagating errors to the main thread,
* so they can be shown to the end user.
*/

class ErrorQueues : public SingletonBase<ErrorQueues> {
public:
    void warn(std::string_view message, bool print = true);
    void error(std::string_view message, bool print = true);
    void success(std::string_view message, bool print = true);
    // notices are messages coming directly from the server
    void notice(std::string_view message, uint32_t replyId, bool print = true);
    void noticeReply(std::string_view message, uint32_t replyId, std::string_view username, bool print = true);

    // debugWarn shows a warn notification in debug, in release only prints a message (noop if `print` = false)
    void debugWarn(std::string_view message, bool print = true);

    bool hasPendingNotices();

    struct PendingNotice {
        std::string message;
        uint32_t replyId;
        std::string isReplyFrom;
    };

    std::vector<std::string> getWarnings();
    std::vector<std::string> getErrors();
    std::vector<std::string> getSuccesses();
    std::vector<PendingNotice> getNotices();
private:
    asp::Channel<std::string> _warns, _errors, _successes;
    asp::Channel<PendingNotice> _notices;
};