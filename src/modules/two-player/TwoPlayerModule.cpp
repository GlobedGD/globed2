#include "TwoPlayerModule.hpp"
#include "ui/LinkRequestPopup.hpp"
#include <globed/core/game/RemotePlayer.hpp>
#include <globed/core/RoomManager.hpp>
#include <globed/core/PopupManager.hpp>
#include <core/hooks/GJBaseGameLayer.hpp>
#include <core/net/NetworkManagerImpl.hpp>
#include <modules/ui/popups/UserListPopup.hpp>

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

TwoPlayerModule::TwoPlayerModule() {
    m_listener = NetworkManagerImpl::get().listenGlobal<msg::LevelDataMessage>([](const auto& msg) {
        auto& mod = TwoPlayerModule::get();

        if (mod.isEnabled()) {
            for (auto& event : msg.events) {
                mod.handleEvent(event);
            }
        }

        return ListenerResult::Continue;
    });
}

void TwoPlayerModule::onModuleInit() {
    this->setAutoEnableMode(AutoEnableMode::Level);
}

Result<> TwoPlayerModule::onDisabled() {
    m_linkedPlayer.reset();
    m_ignoreNoclip = false;
    m_isPlayer2 = false;
    m_linkAttempt.reset();
    m_linkedRp = nullptr;
    return Ok();
}

void TwoPlayerModule::onJoinLevel(GlobedGJBGL* gjbgl, GJGameLevel* level, bool editor) {
    // if 2p mode is disabled, disable the module for this level
    if (!RoomManager::get().getSettings().twoPlayerMode) {
        (void) this->disable();
    } else {
        // safe mode is forced in 2 player mode
        gjbgl->setPermanentSafeMode();
    }
}

void TwoPlayerModule::onPlayerDeath(GlobedGJBGL* gjbgl, RemotePlayer* player, const PlayerDeath& death) {
    if (death.isReal && player && player->displayData().accountId == m_linkedPlayer) {
        gjbgl->killLocalPlayer();
    }
}

void TwoPlayerModule::onUserlistSetup(CCNode* container, int accountId, bool myself, UserListPopup* popup) {
    if (myself) return;

    // if we are already linked, show only an unlink button on the linked player
    if (m_linkedPlayer) {
        if (accountId != *m_linkedPlayer) {
            return;
        }

        Build<CCSprite>::createSpriteName("gj_linkBtnOff_001.png")
            .scale(0.8f)
            .intoMenuItem([this, popup] {
                this->unlink();
                popup->hardRefresh();
            })
            .id("2p-unlink")
            .parent(container);

        return;
    }

    // otherwise, show a link button on all players

    Build<CCSprite>::createSpriteName("gj_linkBtn_001.png")
        .scale(0.8f)
        .intoMenuItem([this, accountId, popup] {
            LinkRequestPopup::create(accountId, popup)->show();
        })
        .id("2p-unlink")
        .parent(container);
}

void TwoPlayerModule::onPlayerLeave(GlobedGJBGL* gjbgl, int accountId) {
    if (accountId == m_linkedPlayer) {
        this->unlink();
    }
}

bool TwoPlayerModule::link(int id, bool player2) {
    if (m_linkAttempt || m_linkedPlayer) {
        globed::toastError("Cannot link while already linked to a player");
        return false;
    }

    m_linkAttempt = id;

    // send an link event
    this->sendLinkEventTo(id, player2);

    return true;
}

void TwoPlayerModule::cancelLink() {
    m_linkAttempt.reset();
}

bool TwoPlayerModule::isLinked() {
    return m_linkedPlayer.has_value();
}

bool TwoPlayerModule::isPlayer2() {
    return m_isPlayer2;
}

bool TwoPlayerModule::waitingForLink() {
    return m_linkAttempt.has_value();
}

VisualPlayer* TwoPlayerModule::getLinkedPlayerObject(bool player2) {
    if (!m_linkedRp) return nullptr;

    return player2 ? m_linkedRp->player2() : m_linkedRp->player1();
}

bool& TwoPlayerModule::ignoreNoclip() {
    return m_ignoreNoclip;
}

void TwoPlayerModule::unlink(bool silent) {
    if (!m_linkedPlayer) return;

    int id = *m_linkedPlayer;
    m_linkedPlayer.reset();

    if (!silent) {
        // send an unlink event
        this->sendUnlinkEventTo(id);
    }
}

void TwoPlayerModule::sendUnlinkEventTo(int id) {
    qn::HeapByteWriter writer;
    writer.writeI32(id);

    NetworkManagerImpl::get().queueGameEvent(Event {
        .type = EVENT_2P_UNLINK,
        .data = std::move(writer).intoVector(),
    });
}

void TwoPlayerModule::sendLinkEventTo(int id, bool player2) {
    qn::HeapByteWriter writer;
    writer.writeI32(id);
    writer.writeBool(!player2);

    NetworkManagerImpl::get().queueGameEvent(Event {
        .type = EVENT_2P_LINK_REQUEST,
        .data = std::move(writer).intoVector(),
    });
}

void TwoPlayerModule::linkSuccess(int id, bool player2) {
    m_linkedPlayer = id;
    m_isPlayer2 = player2;
    m_linkAttempt.reset();

    m_linkedRp = GlobedGJBGL::get()->getPlayer(id);
    if (!m_linkedRp) {
        globed::toastError("(Globed) Failed to find player by ID: {}", id);
        this->unlink();
    }
}

void TwoPlayerModule::handleEvent(const Event& event) {
    switch (event.type) {
        case EVENT_2P_LINK_REQUEST: {
            qn::ByteReader reader{event.data};
            if (auto err = this->handleLinkEvent(reader).err()) {
                log::error("Server sent invalid 2p link event: {}", err->message());
            }
        } break;

        case EVENT_2P_UNLINK: {
            qn::ByteReader reader{event.data};
            if (auto err = this->handleUnlinkEvent(reader).err()) {
                log::error("Server sent invalid 2p unlink event: {}", err->message());
            }
        } break;
    }
}

qn::ByteReader::Result<> TwoPlayerModule::handleLinkEvent(qn::ByteReader& reader) {
    int id = GEODE_UNWRAP(reader.readI32());
    bool player2 = !GEODE_UNWRAP(reader.readBool());

    // are we already linked? if so, ignore this event
    if (m_linkedPlayer) {
        this->sendUnlinkEventTo(id);
        return Ok();
    }

    // are we currently trying to link? check if it's the right user
    if (m_linkAttempt) {
        if (*m_linkAttempt != id) {
            this->sendUnlinkEventTo(id);
            return Ok();
        }

        // successful link!
        this->linkSuccess(id, player2);
        return Ok();
    }

    // if we are not currently trying to link, show a popup to the user, asking if they agree to link
    std::string username;

    if (auto pl = GlobedGJBGL::get()) {
        if (auto player = pl->getPlayer(id)) {
            if (player->isDataInitialized()) {
                username = player->displayData().username;
            }
        }
    }

    if (username.empty()) {
        username = fmt::format("Unknown ({})", id);
    }

    globed::quickPopup(
        "Note",
        fmt::format("<cg>{}</c> wants to link with you. You will be playing as the <cy>{} player</c>. Agree to link?", username, player2 ? "second" : "first"),
        "Cancel",
        "Agree",
        [this, id, player2](auto, bool agree) {
            if (!agree) {
                this->sendUnlinkEventTo(id);
            } else {
                // successful link! make sure to send confirmation event
                this->sendLinkEventTo(id, player2);
                this->linkSuccess(id, player2);
            }
        }
    );

    return Ok();
}

qn::ByteReader::Result<> TwoPlayerModule::handleUnlinkEvent(qn::ByteReader& reader) {
    int id = GEODE_UNWRAP(reader.readI32());

    if (m_linkedPlayer == id) {
        this->unlink(true);
        return Ok();
    }

    // if this is from the current link attempt, cancel the attempt
    if (m_linkAttempt == id) {
        m_linkAttempt.reset();
    }

    return Ok();
}

}
