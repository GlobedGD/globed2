#pragma once
#include <defs.hpp>

#include <functional>

#include "structs.hpp"
#include <util/time.hpp>
#include <util/sync.hpp>

class GHTTPClient;
class GHTTPRequest;

class GHTTPRequestHandle {
public:
    GHTTPRequestHandle(std::shared_ptr<GHTTPRequest> handle) : handle(handle) {}
    // waits for the request to complete (or for timeout to exceed), but doesn't call the callback.
    void discardResult() const;
    // cancels the request outright, the callbacks are guaranteed to not be called after `cancel` has been called at least once.
    // note that it is not possible to cancel a request immediately, there may be up to a one second delay until the actual cancellation happens.
    void cancel() const;
    // calls the callback if the `discardResult` or `cancel` hasn't been called earlier
    void maybeCallback(const GHTTPResponse& response) const;

    std::shared_ptr<GHTTPRequest> handle;
};

class GHTTPRequest {
public:
    using CallbackFunc = std::function<void(const GHTTPResponse&)>;

    // the functions you call at the start
    static GHTTPRequest get();
    static GHTTPRequest get(const std::string_view url);
    static GHTTPRequest post();
    static GHTTPRequest post(const std::string_view url);
    static GHTTPRequest post(const std::string_view url, const std::string_view data);
    static GHTTPRequest put();
    static GHTTPRequest put(const std::string_view url);
    static GHTTPRequest put(const std::string_view url, const std::string_view data);
    static GHTTPRequest delete_();
    static GHTTPRequest delete_(const std::string_view url);

    GHTTPRequest& url(const std::string_view addr);
    GHTTPRequest& userAgent(const std::string_view agent);
    GHTTPRequest& followRedirects(bool follow = true);
    GHTTPRequest& data(const std::string_view dataStr);
    template <typename Rep, typename Period>
    GHTTPRequest& timeout(util::time::duration<Rep, Period> duration) {
        return this->timeout(util::time::asMillis(duration));
    }
    GHTTPRequest& timeout(uint32_t timeoutMs);

    // these two set the Content-Type header
    GHTTPRequest& contentType(const std::string_view ctype);
    GHTTPRequest& contentType(GHTTPContentType ctype);

    // for header() either pass two args for key and value, or one in format "Authorization: xxx"
    GHTTPRequest& header(const std::string_view key, const std::string_view value);
    GHTTPRequest& header(const std::string_view header);

    // set the callback
    GHTTPRequest& then(CallbackFunc cbFunc);

    // Shorthand for GHTTPRequest::send(this)
    GHTTPRequestHandle send(GHTTPClient& client);
    GHTTPRequestHandle send();

    util::sync::AtomicBool _cancelled = false;

protected:
    friend class GHTTPClient;
    friend class GHTTPRequestHandle;

    CallbackFunc callback;
    GHTTPRequestData reqData;
    util::sync::AtomicBool _discard = false;

    GHTTPRequest(GHTTPRequestType type);
    GHTTPRequest();

    // calls the callback
    void callcb(const GHTTPResponse& response) const;
};
