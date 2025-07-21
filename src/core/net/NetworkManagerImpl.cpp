#include "NetworkManagerImpl.hpp"
#include <globed/core/ValueManager.hpp>
#include <argon/argon.hpp>

using namespace geode::prelude;

namespace globed {

NetworkManagerImpl::NetworkManagerImpl() {
    m_centralConn.setConnectionStateCallback([this](qn::ConnectionState state) {
        if (state == qn::ConnectionState::Connected) {
            this->onCentralConnected();
        } else if (state == qn::ConnectionState::Disconnected) {
            this->onCentralDisconnected();
        }
    });

    m_centralConn.setDataCallback([this](std::vector<uint8_t> bytes) {
        qn::ByteReader breader{bytes};
        size_t unpackedSize = breader.readVarUint().unwrapOr(-1);

        kj::ArrayInputStream ais{{bytes.data() + breader.position(), bytes.size()}};
        capnp::PackedMessageReader reader{ais};

        CentralMessage::Reader msg = reader.getRoot<CentralMessage>();

        log::debug("Received {} bytes from central server (msg {})", bytes.size(), (int)msg.which());

        if (auto err = this->onCentralDataReceived(msg).err()) {
            log::error("failed to process message from central server: {}", err);
        }
    });
}

NetworkManagerImpl::~NetworkManagerImpl() {}

Result<> NetworkManagerImpl::connectCentral(std::string_view url) {
    if (m_centralConn.connected()) {
        return Err("Already connected to central server");
    }

    m_centralUrl = std::string(url);
    m_knownArgonUrl.clear();

    return m_centralConn.connect(url).mapErr([](auto&& err) {
        return err.message();
    });
}

Result<> NetworkManagerImpl::disconnectCentral() {
    if (m_waitingForArgon) {
        return Err("cannot disconnect while waiting for Argon auth");
    }

    (void) m_centralConn.disconnect();

    return Ok();
}

void NetworkManagerImpl::onCentralConnected() {
    log::debug("connection to central server established, trying to log in");
    this->tryAuth();
}

void NetworkManagerImpl::tryAuth() {
    auto gam = GJAccountManager::get();
    int accountId = gam->m_accountID;
    int userId = GameManager::get()->m_playerUserID;

    if (auto stoken = this->getUToken()) {
        (void) this->sendToCentral([&](CentralMessage::Builder& msg) {
            log::debug("attempting login with user token");
            auto loginUToken = msg.initLoginUToken();
            loginUToken.setToken(*stoken);
            loginUToken.setAccountId(accountId);
        });
    } else if (!m_knownArgonUrl.empty()) {
        m_waitingForArgon = true;

        auto res = argon::startAuth([&](Result<std::string> res) {
            m_waitingForArgon = false;

            if (!res) {
                this->abortConnection(fmt::format("failed to complete Argon auth: {}", res.unwrapErr()));
                return;
            }

            this->doArgonAuth(*res);
        });

        if (!res) {
            m_waitingForArgon = false;
            this->abortConnection(fmt::format("failed to start Argon auth: {}", res.unwrapErr()));
            return;
        }
    } else {
        (void) this->sendToCentral([&](CentralMessage::Builder& msg) {
            log::debug("attempting plain login");
            auto loginPlain = msg.initLoginPlain();
            auto data = loginPlain.initData();
            data.setUsername(gam->m_username);
            data.setAccountId(accountId);
            data.setUserId(userId);
        });
    }
}

void NetworkManagerImpl::doArgonAuth(std::string token) {
    (void) this->sendToCentral([&](CentralMessage::Builder& msg) {
        log::debug("attempting login with argon token");
        auto loginUToken = msg.initLoginArgon();
        loginUToken.setToken(token);
        loginUToken.setAccountId(GJAccountManager::get()->m_accountID);
    });
}

void NetworkManagerImpl::abortConnection(std::string reason) {
    log::warn("aborting connection to central server: {}", reason);
    (void) m_centralConn.disconnect();
}

void NetworkManagerImpl::onCentralDisconnected() {
    log::debug("connection to central server lost!");
}

Result<> NetworkManagerImpl::onCentralDataReceived(CentralMessage::Reader& msg) {
    switch (msg.which()) {
        case CentralMessage::LOGIN_OK: {
            auto loginOk = msg.getLoginOk();

            if (loginOk.hasNewToken()) {
                std::string newToken = loginOk.getNewToken();
                this->setUToken(newToken);
            }

            if (loginOk.hasServers()) {
                // TODO handle servers
                auto servers = loginOk.getServers();
            }
        } break;

        case CentralMessage::LOGIN_FAILED: {
            auto loginFailed = msg.getLoginFailed();
            this->handleLoginFailed(loginFailed.getReason());
        } break;

        case CentralMessage::LOGIN_REQUIRED: {
            auto loginRequired = msg.getLoginRequired();

            if (!loginRequired.hasArgonUrl()) {
                return Err("Login required message does not contain Argon URL");
            }

            m_knownArgonUrl = loginRequired.getArgonUrl();
            this->clearUToken();
            this->tryAuth();
        } break;
    }

    return Ok();
}

void NetworkManagerImpl::handleLoginFailed(schema::main::LoginFailedReason reason) {
    using enum schema::main::LoginFailedReason;

    switch (reason) {
        case INVALID_USER_TOKEN: {
            log::warn("Login failed: invalid user token, clearing token and trying to re-auth");
            this->clearUToken();
            this->tryAuth();
        } break;

        case INVALID_ARGON_TOKEN: {
            log::warn("Login failed: invalid Argon token, clearing token and trying to re-auth");
            argon::clearToken();
            this->tryAuth();
        } break;

        case ARGON_NOT_SUPPORTED: {
            log::warn("Login failed: Argon is not supported by the server, falling back to plain login");
            m_knownArgonUrl.clear();
            this->tryAuth();
        } break;

        case ARGON_UNREACHABLE:
        case ARGON_INTERNAL_ERROR: {
            log::warn("Login failed: internal server error, argon is unreachable or failing");
            this->abortConnection("Internal server error (auth system is failing)");
        } break;

        default: {
            log::warn("Login failed: unknown reason {}", static_cast<int>(reason));
            this->abortConnection(fmt::format("Login failed due to unknown server error: {}", static_cast<int>(reason)));
        } break;
    }
}

Result<> NetworkManagerImpl::sendToCentral(std::function<void(CentralMessage::Builder&)> func) {
    if (!m_centralConn.connected()) {
        return Err("Not connected to central server");
    }

    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<CentralMessage>();

    func(root);

    size_t unpackedSize = capnp::computeSerializedSizeInWords(msg) * 8;
    qn::HeapByteWriter writer;
    writer.writeVarUint(unpackedSize).unwrap();
    auto unpSizeBuf = writer.written();

    kj::VectorOutputStream vos;
    vos.write(unpSizeBuf.data(), unpSizeBuf.size());
    capnp::writePackedMessage(vos, msg);

    auto data = std::vector<uint8_t>(vos.getArray().begin(), vos.getArray().end());

    m_centralConn.sendData(std::move(data));

    return Ok();
}

std::optional<std::string> NetworkManagerImpl::getUToken() {
    return globed::value<std::string>(fmt::format("auth.last-utoken.{}", m_centralUrl));
}

void NetworkManagerImpl::setUToken(std::string token) {
    ValueManager::get().set(fmt::format("auth.last-utoken.{}", m_centralUrl), std::move(token));
}

void NetworkManagerImpl::clearUToken() {
    ValueManager::get().erase(fmt::format("auth.last-utoken.{}", m_centralUrl));
}

}
