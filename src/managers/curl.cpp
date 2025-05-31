#include "curl.hpp"

#undef _WINSOCKAPI_ // kms tbh
#include <curl/curl.h>

#include <ca_bundle.h>

#include <crypto/chacha_secret_box.hpp>
#include <managers/settings.hpp>
#include <util/crypto.hpp>
#include <util/format.hpp>
#include <util/net.hpp>

constexpr static auto KEY = "bff252d2731a6c6ca26d7f5144bc750fd6723316619f86c8636ebdc13bf3214c";
static ChaChaSecretBox g_box(util::crypto::hexDecode(KEY).unwrap());

struct CurlRequest::Data {
    std::string m_method;
    std::string m_url;
    std::unordered_map<std::string, std::string> m_headers;
    std::unordered_map<std::string, std::string> m_queryParams;
    std::string m_userAgent;
    std::vector<uint8_t> m_body;
    size_t m_timeout = 0;
    bool m_followRedirects = true;
    bool m_encrypt = false;
    bool m_certVerification = true;
};

/* CurlManager */

const char* CurlManager::getCurlVersion() {
    return curl_version_info(CURLVERSION_NOW)->version;
}

CurlManager::Task CurlManager::send(CurlRequest& req) {
    GLOBED_REQUIRE(req.m_data, "attempting to send the same CurlRequest twice");

    return Task::run([data = std::move(req.m_data)](auto, auto hasBeenCancelled) -> Task::Result {
        // init curl
        auto curl = curl_easy_init();
        if (!curl) {
            return CurlResponse::fatalError("curl initialization failed");
        }

        CurlResponse response;

        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.m_rawResponse);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](char* data, size_t size, size_t nmemb, void* userdata) {
            auto& target = *static_cast<std::vector<uint8_t>*>(userdata);
            target.insert(target.end(), data, data + size * nmemb);
            return size * nmemb;
        });

        // set headers
        curl_slist* headers = nullptr;
        for (const auto& [name, value] : data->m_headers) {
            // sanitize header name
            auto hdr = name;
            hdr.erase(std::remove_if(hdr.begin(), hdr.end(), [](char c) {
                return c == '\r' || c == '\n';
            }), hdr.end());
            hdr += ": " + value;
            headers = curl_slist_append(headers, hdr.c_str());
        }

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        auto url = data->m_url;
        bool first = url.find('?') == std::string::npos;
        for (auto& [key, value] : data->m_queryParams) {
            url += (first ? "?" : "&") + util::format::urlEncode(key) + "=" + util::format::urlEncode(value);
            first = false;
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        if (data->m_method != "GET") {
            if (data->m_method == "POST") {
                curl_easy_setopt(curl, CURLOPT_POST, 1L);
            } else if (data->m_method == "HEAD") {
                curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
            } else {
                curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, data->m_method.c_str());
            }
        }

        // transform body if needed
        if (data->m_encrypt && !data->m_body.empty()) {
            size_t plainSize = data->m_body.size();
            data->m_body.resize(plainSize + g_box.prefixLength());
            g_box.encryptInPlace(data->m_body.data(), plainSize);
        }

        // set body
        if (!data->m_body.empty()) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data->m_body.data());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data->m_body.size());
        } else if (data->m_method == "POST") {
            // curl_easy_perform would freeze on a POST request with no fields, so set it to an empty string
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");
        }

        int sslOptions = 0;

        if (data->m_certVerification && !GlobedSettings::get().launchArgs().noSslVerification) {
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

            // Our windows build of curl uses schannel, don't set the cacerts and use system store instead.
            curl_blob cbb = {};
            cbb.data = const_cast<void*>(static_cast<const void*>(CA_BUNDLE_CONTENT));
            cbb.len = sizeof(CA_BUNDLE_CONTENT) - 1;
            cbb.flags = CURL_BLOB_NOCOPY;
            curl_easy_setopt(curl, CURLOPT_CAINFO_BLOB, &cbb);

            // Also add the native CA, for good measure
            sslOptions |= CURLSSLOPT_NATIVE_CA;
        } else {
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        }

        // weird windows stuff, don't remove if we still use schannel!
        GEODE_WINDOWS(sslOptions |= CURLSSLOPT_REVOKE_BEST_EFFORT);
        curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, sslOptions);

        if (!data->m_userAgent.empty()) {
            curl_easy_setopt(curl, CURLOPT_USERAGENT, data->m_userAgent.c_str());
        } else {
            curl_easy_setopt(curl, CURLOPT_USERAGENT, util::net::webUserAgent().c_str());
        }

        if (data->m_timeout != 0) {
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, data->m_timeout);
        }

        // follow redirects
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, data->m_followRedirects ? 1L : 0L);

        // track progress
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

        // don't change the method from POST to GET when following a redirect
        curl_easy_setopt(curl, CURLOPT_POSTREDIR, CURL_REDIR_POST_ALL);

        // do not fail if response code is 4XX or 5XX
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 0L);

        if (GlobedSettings::get().launchArgs().verboseCurl) {
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        }

        char errorBuffer[CURL_ERROR_SIZE];
        errorBuffer[0] = '\0';
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);

        // get headers from the response
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, (+[](char* buffer, size_t size, size_t nitems, void* ptr) {
            auto& headers = static_cast<CurlResponse*>(ptr)->m_headers;
            std::string line;
            std::stringstream ss(std::string(buffer, size * nitems));
            while (std::getline(ss, line)) {
                auto colon = line.find(':');
                if (colon == std::string::npos) continue;
                auto key = line.substr(0, colon);
                auto value = line.substr(colon + 2);
                if (value.ends_with('\r')) {
                    value = value.substr(0, value.size() - 1);
                }
                headers.insert_or_assign(key, value);
            }
            return size * nitems;
        }));


        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &hasBeenCancelled);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, +[](void* ptr, curl_off_t dtotal, curl_off_t dnow, curl_off_t utotal, curl_off_t unow) -> int {
            auto& hbc = *static_cast<decltype(hasBeenCancelled)*>(ptr);

            // check if request was cancelled
            if (hbc()) {
                return 1;
            }

            return 0;
        });

        // perform the request
        CURLcode code = curl_easy_perform(curl);

        if (code == CURLE_OK) {
            long status = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
            response.m_code = status;
        } else {
            if (hasBeenCancelled()) {
                curl_slist_free_all(headers);
                curl_easy_cleanup(curl);
                return Task::Cancel();
            }

            std::string_view providedMessage{errorBuffer};

            response.m_code = 0;

            if (providedMessage.empty()) {
                response.m_fatalMessage = fmt::format("Curl failed: {}", curl_easy_strerror(code));
            } else {
                response.m_fatalMessage = fmt::format("Curl failed ({}): {}", (int)code, providedMessage);
            }
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        return response;
    }, fmt::format("CurlManager web request"));
}

/* CurlRequest */

CurlRequest::CurlRequest() : m_data(std::make_shared<Data>()) {}

CurlRequest& CurlRequest::header(std::string_view name, std::string_view value) {
    if (name == "User-Agent" || name == "user-agent") {
        this->userAgent(value);
        return *this;
    }

    m_data->m_headers.insert_or_assign(std::string(name), std::string(value));
    return *this;
}

CurlRequest& CurlRequest::param(std::string_view name, std::string_view value) {
    m_data->m_queryParams.insert_or_assign(std::string(name), std::string(value));
    return *this;
}

CurlRequest& CurlRequest::userAgent(std::string_view name) {
    m_data->m_userAgent = name;
    return *this;
}

CurlRequest& CurlRequest::timeout(size_t seconds) {
    m_data->m_timeout = seconds;
    return *this;
}

CurlRequest& CurlRequest::followRedirects(bool follow) {
    m_data->m_followRedirects = follow;
    return *this;
}

CurlRequest& CurlRequest::body(const std::vector<uint8_t>& data) {
    m_data->m_body = data;
    return *this;
}

CurlRequest& CurlRequest::body(std::vector<uint8_t>&& data) {
    m_data->m_body = std::move(data);
    return *this;
}

CurlRequest& CurlRequest::body(std::string_view str) {
    auto sptr = reinterpret_cast<const uint8_t*>(str.data());
    std::vector<uint8_t> bytes(sptr, sptr + str.size());

    return this->body(std::move(bytes));
}

CurlRequest& CurlRequest::bodyJSON(const matjson::Value& json) {
    this->header("Content-Type", "application/json");
    return this->body(json.dump(matjson::NO_INDENTATION));
}

CurlRequest& CurlRequest::get(std::string_view url) {
    return this->customMethod(url, "GET");
}

CurlRequest& CurlRequest::post(std::string_view url) {
    return this->customMethod(url, "POST");
}

CurlRequest& CurlRequest::put(std::string_view url) {
    return this->customMethod(url, "PUT");
}

CurlRequest& CurlRequest::patch(std::string_view url) {
    return this->customMethod(url, "PATCH");
}

CurlRequest& CurlRequest::delete_(std::string_view url) {
    return this->customMethod(url, "DELETE");
}

CurlRequest& CurlRequest::head(std::string_view url) {
    return this->customMethod(url, "HEAD");
}

CurlRequest& CurlRequest::customMethod(std::string_view url, std::string_view method) {
    m_data->m_method = method;
    m_data->m_url = url;
    return *this;
}

CurlRequest& CurlRequest::encrypted(bool enc) {
    m_data->m_encrypt = enc;
    return *this;
}

CurlRequest& CurlRequest::certVerification(bool enc) {
    m_data->m_certVerification = enc;
    return *this;
}

CurlManager::Task CurlRequest::send() {
    return CurlManager::get().send(*this);
}

/* CurlResponse */

CurlResponse CurlResponse::fatalError(std::string_view message) {
    CurlResponse resp;
    resp.m_code = 0;
    resp.m_fatalMessage = message;
    return resp;
}

int CurlResponse::getCode() const {
    return m_code;
}

bool CurlResponse::ok() const {
    return m_fatalMessage.empty() && m_code >= 200 && m_code < 300;
}

std::string CurlResponse::header(std::string_view key) const {
    std::string k(key);
    if (m_headers.contains(k)) {
        return m_headers.at(k);
    }

    return "";
}

const std::unordered_map<std::string, std::string>& CurlResponse::headers(std::string_view key) const {
    return m_headers;
}

geode::Result<std::vector<uint8_t>> CurlResponse::data() {
    if (!m_fatalMessage.empty()) {
        return Err(m_fatalMessage);
    }

    return Ok(m_rawResponse);
}

geode::Result<std::string> CurlResponse::text() {
    if (!m_fatalMessage.empty()) {
        return Err(m_fatalMessage);
    }

    return Ok(std::string(&*m_rawResponse.begin(), &*m_rawResponse.end()));
}

geode::Result<matjson::Value> CurlResponse::json() {
    GLOBED_UNWRAP_INTO(this->text(), auto str);

    auto val = matjson::parse(str);
    if (!val) {
        return Err((std::string) val.unwrapErr());
    }

    return Ok(std::move(val.unwrap()));
}

std::string CurlResponse::getError() {
    if (!m_fatalMessage.empty()) {
        return fmt::format("Fatal error: {}", m_fatalMessage);
    } else {
        return fmt::format("code {}: {}", m_code, this->text().unwrapOr("<no content>"));
    }
}