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
    void discardResult() const;
    // calls the callback if the `discardResult` hasn't been called earlier
    void maybeCallback(const GHTTPResponse& response) const;

    std::shared_ptr<GHTTPRequest> handle;
};

class GHTTPRequest {
public:
    using CallbackFunc = std::function<void(const GHTTPResponse&)>;

    // the functions you call at the start
    static GHTTPRequest get();
    static GHTTPRequest get(const std::string& url);
    static GHTTPRequest post();
    static GHTTPRequest post(const std::string& url);
    static GHTTPRequest post(const std::string& url, const std::string& data);
    static GHTTPRequest put();
    static GHTTPRequest put(const std::string& url);
    static GHTTPRequest put(const std::string& url, const std::string& data);
    static GHTTPRequest delete_();
    static GHTTPRequest delete_(const std::string& url);

    GHTTPRequest& url(const std::string& addr);
    GHTTPRequest& userAgent(const std::string& agent);
    GHTTPRequest& followRedirects(bool follow = true);
    GHTTPRequest& data(const std::string& dataStr);
    template <typename Rep, typename Period>
    GHTTPRequest& timeout(util::time::duration<Rep, Period> duration) {
        return this->timeout(util::time::asMillis(duration));
    }
    GHTTPRequest& timeout(uint32_t timeoutMs);

    // these two set the Content-Type header
    GHTTPRequest& contentType(const std::string& ctype);
    GHTTPRequest& contentType(GHTTPContentType ctype);

    // for header() either pass two args for key and value, or one in format "Authorization: xxx"
    GHTTPRequest& header(const std::string& key, const std::string& value);
    GHTTPRequest& header(const std::string& header);

    // set the callback
    GHTTPRequest& then(CallbackFunc cbFunc);

    // Shorthand for GHTTPRequest::send(this)
    GHTTPRequestHandle send(GHTTPClient& client);
    GHTTPRequestHandle send();

protected:
    friend class GHTTPClient;
    friend class GHTTPRequestHandle;

    CallbackFunc callback;
    GHTTPRequestType rType;
    std::string rUrl, rUserAgent = "GHTTPClient/1.0", rData;
    uint32_t rTimeout = 0;
    std::vector<std::string> rHeaders;
    bool rFollowRedirects = true;
    util::sync::AtomicBool _discard = false;

    GHTTPRequest(GHTTPRequestType type);
    GHTTPRequest();

    // calls the callback
    void callcb(const GHTTPResponse& response) const;
};
