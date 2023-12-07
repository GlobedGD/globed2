#pragma once
#include <defs.hpp>

#include <queue>
#include <thread>

#include "request.hpp"
#include <util/sync.hpp>

class GHTTPClient {
    GLOBED_SINGLETON(GHTTPClient)
    GHTTPClient();
    ~GHTTPClient();

    // add the request to the queue of pending requests
    void send(GHTTPRequestHandle request);

protected:
    CURL* curl;
    std::thread threadHandle;

    util::sync::AtomicBool _running = true;
    util::sync::SmartMessageQueue<GHTTPRequestHandle> requests;

    void threadFunc();

    GHTTPResponse performRequest(GHTTPRequestHandle req);
};