#pragma once

#include <Geode/cocos/platform/IncludeCurl.h>

struct GHTTPResponse {
    // curlcode indicating the result, is always set
    CURLcode resCode;

    // failed is *only* true when a cURL error happened. If the server responded with 4xx or 5xx, it is false.
    bool failed;
    // is set to be a human readable string if `failed == true`
    std::string failMessage;

    // Is set to the HTTP status code if `failed == false`, otherwise is set to 0.
    int statusCode = 0;
    // is set to be a response if `failed == false`, otherwise is empty
    std::string response;

    // returns `true` if any kind of failure occurred, whether a curl error or a non 2xx status code
    bool anyfail() const {
        return failed || !(statusCode >= 200 && statusCode < 300);
    }

    // returns `failMessage` if a curl error occurred, otherwise `response` if the status code isn't 2xx
    std::string anyfailmsg() const {
        return failed ? failMessage : response;
    }
};

enum class GHTTPRequestType {
    Get, Post, Put, Delete
};

enum class GHTTPContentType {
    FormUrlencoded,
    Data,
    Json,
    Text
};

struct GHTTPRequestData {
    GHTTPRequestType reqType;
    std::string url, userAgent = "GHTTPClient/1.0", payload;
    uint32_t timeout = 0;
    std::vector<std::string> headers;
    bool followRedirects = true;
};
