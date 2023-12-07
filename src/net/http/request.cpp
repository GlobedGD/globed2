#include "request.hpp"

#include "client.hpp"

void GHTTPRequestHandle::discardResult() const {
    handle->_discard = true;
}

void GHTTPRequestHandle::maybeCallback(const GHTTPResponse& response) const {
    if (!handle->_discard) {
        handle->callcb(response);
    }
}

void GHTTPRequest::callcb(const GHTTPResponse& response) const {
    callback(response);
}

GHTTPRequest::GHTTPRequest(GHTTPRequestType type) : rType(type) {}
GHTTPRequest::GHTTPRequest() {}

GHTTPRequest GHTTPRequest::get() {
    return GHTTPRequest(GHTTPRequestType::GET);
}

GHTTPRequest GHTTPRequest::get(const std::string& url) {
    auto req = GHTTPRequest(GHTTPRequestType::GET);
    req.rUrl = url;
    return req;
}

GHTTPRequest GHTTPRequest::post() {
    return GHTTPRequest(GHTTPRequestType::POST);
}

GHTTPRequest GHTTPRequest::post(const std::string& url) {
    auto req = GHTTPRequest(GHTTPRequestType::POST);
    req.rUrl = url;
    return req;
}

GHTTPRequest GHTTPRequest::post(const std::string& url, const std::string& data) {
    auto req = GHTTPRequest(GHTTPRequestType::POST);
    req.rUrl = url;
    req.rData = data;
    return req;
}

GHTTPRequest GHTTPRequest::put() {
    return GHTTPRequest(GHTTPRequestType::PUT);
}

GHTTPRequest GHTTPRequest::put(const std::string& url) {
    auto req = GHTTPRequest(GHTTPRequestType::PUT);
    req.rUrl = url;
    return req;
}

GHTTPRequest GHTTPRequest::put(const std::string& url, const std::string& data) {
    auto req = GHTTPRequest(GHTTPRequestType::PUT);
    req.rUrl = url;
    req.rData = data;
    return req;
}

GHTTPRequest GHTTPRequest::delete_() {
    return GHTTPRequest(GHTTPRequestType::DELETE_);
}

GHTTPRequest GHTTPRequest::delete_(const std::string& url) {
    auto req = GHTTPRequest(GHTTPRequestType::DELETE_);
    req.rUrl = url;
    return req;
}

GHTTPRequest& GHTTPRequest::url(const std::string& addr) {
    rUrl = addr;
    return *this;
}

GHTTPRequest& GHTTPRequest::userAgent(const std::string& agent) {
    rUserAgent = agent;
    return *this;
}

GHTTPRequest& GHTTPRequest::followRedirects(bool follow) {
    rFollowRedirects = follow;
    return *this;
}

GHTTPRequest& GHTTPRequest::data(const std::string& dataStr) {
    rData = dataStr;
    return *this;
}

GHTTPRequest& GHTTPRequest::timeout(uint32_t timeoutMs) {
    rTimeout = timeoutMs;
    return *this;
}

GHTTPRequest& GHTTPRequest::contentType(const std::string& ctype) {
    return this->header("Content-Type", ctype);
}

GHTTPRequest& GHTTPRequest::contentType(GHTTPContentType ctype) {
    const char* headerValue;
    switch (ctype) {
    case GHTTPContentType::FORM_URLENCODED:
        headerValue = "application/x-www-form-urlencoded";
        break;
    case GHTTPContentType::JSON:
        headerValue = "application/json";
        break;
    case GHTTPContentType::TEXT:
        headerValue = "text/plain";
        break;
    case GHTTPContentType::DATA:
        headerValue = "application/octet-stream";
        break;
    }

    return this->header("Content-Type", headerValue);
}

GHTTPRequest& GHTTPRequest::header(const std::string& key, const std::string& value) {
    return this->header(key + ": " + value);
}

GHTTPRequest& GHTTPRequest::header(const std::string& header) {
    rHeaders.push_back(header);
    return *this;
}

GHTTPRequest& GHTTPRequest::then(CallbackFunc cbFunc) {
    callback = cbFunc;
    return *this;
}

GHTTPRequestHandle GHTTPRequest::send(GHTTPClient& client) {
    auto sptr = std::make_shared<GHTTPRequest>(*this);
    auto handle = GHTTPRequestHandle(sptr);

    client.send(handle);
    return handle;
}

GHTTPRequestHandle GHTTPRequest::send() {
    return this->send(GHTTPClient::get());
}
