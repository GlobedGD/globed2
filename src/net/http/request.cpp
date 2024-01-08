#include "request.hpp"

#include "client.hpp"

void GHTTPRequestHandle::discardResult() const {
    handle->_discard = true;
}

void GHTTPRequestHandle::cancel() const {
    this->discardResult();
    handle->_cancelled = true;
}

void GHTTPRequestHandle::maybeCallback(const GHTTPResponse& response) const {
    if (!handle->_discard) {
        handle->callcb(response);
    }
}

void GHTTPRequest::callcb(const GHTTPResponse& response) const {
    callback(response);
}

GHTTPRequest::GHTTPRequest(GHTTPRequestType type) {
    reqData.reqType = type;
}

GHTTPRequest::GHTTPRequest() {}

GHTTPRequest GHTTPRequest::get() {
    return GHTTPRequest(GHTTPRequestType::Get);
}

GHTTPRequest GHTTPRequest::get(const std::string_view url) {
    auto req = GHTTPRequest(GHTTPRequestType::Get);
    req.reqData.url = url;
    return req;
}

GHTTPRequest GHTTPRequest::post() {
    return GHTTPRequest(GHTTPRequestType::Post);
}

GHTTPRequest GHTTPRequest::post(const std::string_view url) {
    auto req = GHTTPRequest(GHTTPRequestType::Post);
    req.reqData.url = url;
    return req;
}

GHTTPRequest GHTTPRequest::post(const std::string_view url, const std::string_view data) {
    auto req = GHTTPRequest(GHTTPRequestType::Post);
    req.reqData.url = url;
    req.reqData.payload = data;
    return req;
}

GHTTPRequest GHTTPRequest::put() {
    return GHTTPRequest(GHTTPRequestType::Put);
}

GHTTPRequest GHTTPRequest::put(const std::string_view url) {
    auto req = GHTTPRequest(GHTTPRequestType::Put);
    req.reqData.url = url;
    return req;
}

GHTTPRequest GHTTPRequest::put(const std::string_view url, const std::string_view data) {
    auto req = GHTTPRequest(GHTTPRequestType::Put);
    req.reqData.url = url;
    req.reqData.payload = data;
    return req;
}

GHTTPRequest GHTTPRequest::delete_() {
    return GHTTPRequest(GHTTPRequestType::Delete);
}

GHTTPRequest GHTTPRequest::delete_(const std::string_view url) {
    auto req = GHTTPRequest(GHTTPRequestType::Delete);
    req.reqData.url = url;
    return req;
}

GHTTPRequest& GHTTPRequest::url(const std::string_view addr) {
    reqData.url = addr;
    return *this;
}

GHTTPRequest& GHTTPRequest::userAgent(const std::string_view agent) {
    reqData.userAgent = agent;
    return *this;
}

GHTTPRequest& GHTTPRequest::followRedirects(bool follow) {
    reqData.followRedirects = follow;
    return *this;
}

GHTTPRequest& GHTTPRequest::data(const std::string_view dataStr) {
    reqData.payload = dataStr;
    return *this;
}

GHTTPRequest& GHTTPRequest::timeout(uint32_t timeoutMs) {
    reqData.timeout = timeoutMs;
    return *this;
}

GHTTPRequest& GHTTPRequest::contentType(const std::string_view ctype) {
    return this->header("Content-Type", ctype);
}

GHTTPRequest& GHTTPRequest::contentType(GHTTPContentType ctype) {
    const char* headerValue;
    switch (ctype) {
    case GHTTPContentType::FormUrlencoded:
        headerValue = "application/x-www-form-urlencoded";
        break;
    case GHTTPContentType::Json:
        headerValue = "application/json";
        break;
    case GHTTPContentType::Text:
        headerValue = "text/plain";
        break;
    case GHTTPContentType::Data:
        headerValue = "application/octet-stream";
        break;
    }

    return this->header("Content-Type", headerValue);
}

GHTTPRequest& GHTTPRequest::header(const std::string_view key, const std::string_view value) {
    return this->header(fmt::format("{}: {}", key, value));
}

GHTTPRequest& GHTTPRequest::header(const std::string_view header) {
    reqData.headers.push_back(std::string(header));
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
