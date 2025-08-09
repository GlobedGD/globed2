#pragma once

#include "generated.hpp"
#include <globed/core/data/RoomSettings.hpp>
#include <globed/core/data/Messages.hpp>
#include <globed/core/data/UserRole.hpp>

namespace globed::data {

/// Player state

inline void encodePlayerState(const PlayerState& state, auto&& data) {
    data.setAccountId(state.accountId);
    data.setTimestamp(state.timestamp);
    data.setFrameNumber(state.frameNumber);
    data.setDeathCount(state.deathCount);
    data.setPercentage(state.percentage);
    data.setIsDead(state.isDead);
    data.setIsPaused(state.isPaused);
    data.setIsPracticing(state.isPracticing);
    data.setIsInEditor(state.isInEditor);
    data.setIsEditorBuilding(state.isEditorBuilding);
    data.setIsLastDeathReal(state.isLastDeathReal);

    auto initPlayer = [](PlayerObjectData src, auto dst) {
        dst.setPositionX(src.position.x);
        dst.setPositionY(src.position.y);
        dst.setRotation(src.rotation);
        dst.setIconType(static_cast<::globed::schema::shared::IconType>(src.iconType));

        dst.setIsVisible(src.isVisible);
        dst.setIsLookingLeft(src.isLookingLeft);
        dst.setIsUpsideDown(src.isUpsideDown);
        dst.setIsDashing(src.isDashing);
        dst.setIsMini(src.isMini);
        dst.setIsGrounded(src.isGrounded);
        dst.setIsStationary(src.isStationary);
        dst.setIsFalling(src.isFalling);
        dst.setIsRotating(src.isRotating);
        dst.setIsSideways(src.isSideways);
    };

    if (state.player2) {
        auto dual = data.initDual();
        auto p1 = dual.initPlayer1();
        auto p2 = dual.initPlayer2();
        initPlayer(state.player1, p1);
        initPlayer(*state.player2, p2);
    } else {
        auto single = data.initSingle();
        auto p1 = single.initPlayer1();
        initPlayer(state.player1, p1);
    }
}

inline std::optional<PlayerState> decodePlayerState(const schema::game::PlayerData::Reader& reader) {
    PlayerState out{};
    out.accountId = reader.getAccountId();
    out.timestamp = reader.getTimestamp();
    out.frameNumber = reader.getFrameNumber();
    out.deathCount = reader.getDeathCount();
    out.percentage = reader.getPercentage();
    out.isDead = reader.getIsDead();
    out.isPaused = reader.getIsPaused();
    out.isPracticing = reader.getIsPracticing();
    out.isInEditor = reader.getIsInEditor();
    out.isEditorBuilding = reader.getIsEditorBuilding();
    out.isLastDeathReal = reader.getIsLastDeathReal();

    auto initPlayer = [](auto&& src, PlayerObjectData& dst) {
        dst.position.x = src.getPositionX();
        dst.position.y = src.getPositionY();
        dst.rotation = src.getRotation();
        dst.iconType = static_cast<PlayerIconType>(src.getIconType());

        dst.isVisible = src.getIsVisible();
        dst.isLookingLeft = src.getIsLookingLeft();
        dst.isUpsideDown = src.getIsUpsideDown();
        dst.isDashing = src.getIsDashing();
        dst.isMini = src.getIsMini();
        dst.isGrounded = src.getIsGrounded();
        dst.isStationary = src.getIsStationary();
        dst.isFalling = src.getIsFalling();
        dst.isRotating = src.getIsRotating();
        dst.isSideways = src.getIsSideways();
    };

    if (reader.isDual()) {
        auto dual = reader.getDual();
        if (!dual.hasPlayer1() || !dual.hasPlayer2()) {
            return std::nullopt;
        }

        auto p1 = dual.getPlayer1();
        auto p2 = dual.getPlayer2();

        out.player2 = PlayerObjectData{};
        initPlayer(p1, out.player1);
        initPlayer(p2, *out.player2);
    } else if (reader.isSingle()) {
        auto single = reader.getSingle();
        if (!single.hasPlayer1()) {
            return std::nullopt;
        }

        auto p1 = single.getPlayer1();
        out.player2 = std::nullopt;
        initPlayer(p1, out.player1);
    } else {
        return std::nullopt;
    }

    return out;
}

// Icon data

inline void encodeIconData(const PlayerIconData& icons, auto&& data) {
    data.setCube(icons.cube);
    data.setShip(icons.ship);
    data.setBall(icons.ball);
    data.setUfo(icons.ufo);
    data.setWave(icons.wave);
    data.setRobot(icons.robot);
    data.setSpider(icons.spider);
    data.setSwing(icons.swing);
    data.setJetpack(icons.jetpack);
    data.setColor1(icons.color1);
    data.setColor2(icons.color2);
    data.setGlowColor(icons.glowColor);
    data.setDeathEffect(icons.deathEffect);
    data.setTrail(icons.trail);
    data.setShipTrail(icons.shipTrail);
}

inline std::optional<PlayerIconData> decodeIconData(const schema::shared::PlayerIconData::Reader& reader) {
    PlayerIconData out{};
    out.cube = reader.getCube();
    out.ship = reader.getShip();
    out.ball = reader.getBall();
    out.ufo = reader.getUfo();
    out.wave = reader.getWave();
    out.robot = reader.getRobot();
    out.spider = reader.getSpider();
    out.swing = reader.getSwing();
    out.jetpack = reader.getJetpack();
    out.color1 = reader.getColor1();
    out.color2 = reader.getColor2();
    out.glowColor = reader.getGlowColor();
    out.deathEffect = reader.getDeathEffect();
    out.trail = reader.getTrail();
    out.shipTrail = reader.getShipTrail();
    return out;
}

// Room settings

inline void encodeRoomSettings(const RoomSettings& settings, auto&& data) {
    data.setPlayerLimit(settings.playerLimit);
    data.setFasterReset(settings.fasterReset);
    data.setHidden(settings.hidden);
    data.setPrivateInvites(settings.privateInvites);
    data.setIsFollower(settings.isFollower);
    data.setLevelIntegrity(settings.levelIntegrity);
    data.setCollision(settings.collision);
    data.setTwoPlayerMode(settings.twoPlayerMode);
    data.setDeathlink(settings.deathlink);
}

inline RoomSettings decodeRoomSettings(const schema::main::RoomSettings::Reader& reader) {
    RoomSettings out{};
    out.playerLimit = reader.getPlayerLimit();
    out.fasterReset = reader.getFasterReset();
    out.hidden = reader.getHidden();
    out.privateInvites = reader.getPrivateInvites();
    out.isFollower = reader.getIsFollower();
    out.levelIntegrity = reader.getLevelIntegrity();
    out.collision = reader.getCollision();
    out.twoPlayerMode = reader.getTwoPlayerMode();
    out.deathlink = reader.getDeathlink();
    return out;
}

// Display data

inline std::optional<PlayerDisplayData> decodeDisplayData(const schema::shared::PlayerDisplayData::Reader& reader) {
    PlayerDisplayData out{};
    out.accountId = reader.getAccountId();

    if (out.accountId == 0) {
        return std::nullopt;
    }

    out.userId = reader.getUserId();
    out.username = reader.getUsername();

    auto iconsR = reader.getIcons();

    if (auto icons = decodeIconData(iconsR)) {
        out.icons = *icons;
    } else {
        return std::nullopt;
    }

    return out;
}

// Account data

inline PlayerAccountData decodeAccountData(const schema::main::PlayerAccountData::Reader& reader) {
    PlayerAccountData out{};
    out.accountId = reader.getAccountId();
    out.userId = reader.getUserId();
    out.username = reader.getUsername();

    return out;
}

// Room player

inline RoomPlayer decodeRoomPlayer(const schema::main::RoomPlayer::Reader& reader) {
    RoomPlayer out{};
    out.cube = reader.getCube();
    out.color1 = reader.getColor1();
    out.color2 = reader.getColor2();
    out.glowColor = reader.getGlowColor();
    out.session = SessionId(reader.getSession());

    auto accountDataR = reader.getAccountData();
    out.accountData = decodeAccountData(accountDataR);

    return out;
}

// Room state

inline msg::RoomStateMessage decodeRoomStateMessage(schema::main::RoomStateMessage::Reader& reader) {
    msg::RoomStateMessage out{};
    out.roomId = reader.getRoomId();
    out.roomOwner = reader.getRoomOwner();
    out.roomName = reader.getRoomName();

    auto players = reader.getPlayers();
    out.players.reserve(players.size());

    for (auto player : players) {
        out.players.push_back(decodeRoomPlayer(player));
    }

    out.settings = data::decodeRoomSettings(reader.getSettings());

    return out;
}

// Room join failed

inline std::optional<msg::RoomJoinFailedMessage> decodeRoomJoinFailedMessage(schema::main::RoomJoinFailedMessage::Reader& reader) {
    using enum schema::main::RoomJoinFailedReason;

    msg::RoomJoinFailedMessage out{};
    out.reason = static_cast<msg::RoomJoinFailedReason>(reader.getReason());

    return out.reason <= msg::RoomJoinFailedReason::Last_ ? std::optional{out} : std::nullopt;
}

// Room create failed

inline std::optional<msg::RoomCreateFailedMessage> decodeRoomCreateFailedMessage(schema::main::RoomCreateFailedMessage::Reader& reader) {
    using enum schema::main::RoomCreateFailedReason;

    msg::RoomCreateFailedMessage out{};
    out.reason = static_cast<msg::RoomCreateFailedReason>(reader.getReason());

    return out.reason <= msg::RoomCreateFailedReason::Last_ ? std::optional{out} : std::nullopt;
}

// Room listing

inline std::optional<msg::RoomListMessage> decodeRoomListMessage(schema::main::RoomListMessage::Reader& reader) {
    msg::RoomListMessage out{};
    auto rooms = reader.getRooms();
    out.rooms.reserve(rooms.size());

    for (auto room : rooms) {
        globed::RoomListingInfo info{};

        info.roomId = room.getRoomId();
        info.roomName = room.getRoomName();
        info.roomOwner = decodeRoomPlayer(room.getRoomOwner());
        info.playerCount = room.getPlayerCount();
        info.hasPassword = room.getHasPassword();
        info.settings = data::decodeRoomSettings(room.getSettings());

        out.rooms.push_back(std::move(info));
    }

    return std::optional{out};
}

// Level data

inline msg::LevelDataMessage decodeLevelDataMessage(const schema::game::LevelDataMessage::Reader& reader) {
    auto players = reader.getPlayers();
    auto culled = reader.getCulled();
    auto ddatas = reader.getDisplayDatas();
    auto events = reader.getEvents();

    msg::LevelDataMessage outMsg;
    outMsg.players.reserve(players.size());
    outMsg.culled.reserve(culled.size());
    outMsg.displayDatas.reserve(ddatas.size());
    outMsg.events.reserve(events.size());

    for (auto player : players) {
        if (auto s = data::decodePlayerState(player)) {
            outMsg.players.emplace_back(*std::move(s));
        } else {
            geode::log::warn("Server sent invalid player state data for {}, skipping", player.getAccountId());
        }
    }

    for (auto id : culled) {
        outMsg.culled.push_back(id);
    }

    for (auto dd : ddatas) {
        if (auto s = data::decodeDisplayData(dd)) {
            outMsg.displayDatas.push_back(*std::move(s));
        } else {
            // can happen as an optimization
            geode::log::debug("Server sent invalid player display data, skipping");
        }
    }

    for (auto event : events) {
        auto data = event.getData().asBytes();

        Event ev;
        ev.type = static_cast<uint16_t>(event.getType());
        ev.data = std::vector<uint8_t>(data.begin(), data.end());
        outMsg.events.push_back(std::move(ev));
    }

    return outMsg;
}

/// User roles

inline UserRole decodeUserRole(const schema::shared::UserRole::Reader& reader, size_t idx) {
    UserRole role{};
    role.id = idx;
    role.stringId = reader.getStringId();
    role.icon = reader.getIcon();
    role.nameColor = reader.getNameColor();
    return role;
}

/// Central login ok message

inline msg::CentralLoginOkMessage decodeCentralLoginOk(const schema::main::LoginOkMessage::Reader& reader) {
    msg::CentralLoginOkMessage msg{};

    if (reader.hasNewToken()) {
        msg.newToken = reader.getNewToken();
    }

    if (reader.hasAllRoles()) {
        auto roles = reader.getAllRoles();
        msg.allRoles.reserve(roles.size());

        size_t idx = 0;
        for (auto role : roles) {
            msg.allRoles.push_back(decodeUserRole(role, idx++));
        }
    }

    if (reader.hasUserRoles()) {
        auto userRoles = reader.getUserRoles();
        msg.userRoles.reserve(userRoles.size());
        for (uint8_t id : userRoles) {
            if (id < msg.allRoles.size()) {
                msg.userRoles.push_back(msg.allRoles[id]);
            } else {
                geode::log::warn("Received invalid user role ID: {}", id);
            }
        }
    }

    msg.isModerator = reader.getIsModerator();

    return msg;
}

}