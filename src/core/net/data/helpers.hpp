#pragma once

#include "generated.hpp"
#include <globed/core/data/RoomSettings.hpp>
#include <globed/core/data/Messages.hpp>
#include <globed/core/data/UserRole.hpp>
#include <globed/util/assert.hpp>
#include <modules/scripting/data/EmbeddedScript.hpp>

#include <qunet/buffers/ByteReader.hpp>
#include <capnp/compat/std-iterator.h>

namespace globed::data {

/// Color conversion

inline uint32_t encodeColor4(cocos2d::ccColor4B color) {
    return (
        (uint32_t)color.r << 24
        | (uint32_t)color.g << 16
        | (uint32_t)color.b << 8
        | (uint32_t)color.a
    );
}

inline cocos2d::ccColor4B decodeColor4(uint32_t color) {
    return cocos2d::ccColor4B {
        .r = uint8_t(color >> 24),
        .g = uint8_t(color >> 16),
        .b = uint8_t(color >> 8),
        .a = uint8_t(color),
    };
}

template <typename CxxS, typename CapnpW>
struct Encode {
    static void encode(const CxxS&, CapnpW&&) {
        static_assert(std::is_void_v<CxxS>, "encode not implemented for this type");
    }
};

template <typename CxxS, typename CapnpR>
struct Decode {
    static std::optional<CxxS> decodeOpt(const CapnpR&) {
        static_assert(std::is_void_v<CxxS>, "decode not implemented for this type");
    }
};

template <typename CxxS, typename CapnpW>
static void encode(const CxxS& s, CapnpW&& w) {
    Encode<CxxS, CapnpW>::encode(s, std::forward<CapnpW>(w));
}

template <typename CxxS, typename CapnpR>
static std::optional<CxxS> decodeOpt(const CapnpR& r) {
    return Decode<CxxS, std::decay_t<CapnpR>>::decodeOpt(r);
}
template <typename CxxS, typename CapnpR>
static CxxS decodeUnchecked(const CapnpR& r) {
    return std::move(*decodeOpt<CxxS, CapnpR>(r));
}

template <typename A>
struct ExtractFnArg;

template <typename Z>
struct ExtractFnArg<std::function<void(Z)>> {
    using type = Z;
};

/// Extracts the type of a parameter expression declaration.
/// For example: `const int& x` -> `const int&`
#define EXTRACT_EXPR_TYPE(expr) ExtractFnArg<std::function<void(expr)>>::type

#define $implEncodeInner(cxxt, wrt) \
    template <typename Y> \
    struct Encode<std::decay_t<EXTRACT_EXPR_TYPE(cxxt)>, Y> { \
        using Wrt = std::decay_t<EXTRACT_EXPR_TYPE(wrt)>; \
        static void encode(std::decay_t<EXTRACT_EXPR_TYPE(cxxt)> _zencodeInnervar, Y&& y) { encodeInner(_zencodeInnervar, y); } \
        static void encodeInner(cxxt, wrt); \
    }; \
    template <typename Y> \
    void Encode<std::decay_t<EXTRACT_EXPR_TYPE(cxxt)>, Y>::encodeInner(cxxt, wrt)

#define $implDecodeInner(cxxt, rt) \
    template <> \
    struct Decode<cxxt, std::decay_t<EXTRACT_EXPR_TYPE(rt)>> { \
        static std::optional<cxxt> decodeOpt(rt); \
    }; \
    inline std::optional<cxxt> Decode<cxxt, std::decay_t<EXTRACT_EXPR_TYPE(rt)>>::decodeOpt(rt)

#define $implEncode(cxxt, wrt) $implEncodeInner(cxxt, ::globed::schema::wrt)
#define $implDecode(cxxt, rt) $implDecodeInner(cxxt, const ::globed::schema::rt)

$implEncode(const PlayerState& state, game::PlayerData::Builder& data) {
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

        if (src.extData) {
            auto ed = *src.extData;
            auto ext = dst.initExtData();
            ext.setVelocityX(ed.velocityX);
            ext.setVelocityY(ed.velocityY);
            ext.setAccelerating(ed.accelerating);
            ext.setAcceleration(ed.acceleration);
            ext.setFallStartY(ed.fallStartY);
            ext.setIsOnGround2(ed.isOnGround2);
            ext.setGravityMod(ed.gravityMod);
            ext.setGravity(ed.gravity);
            ext.setTouchedPad(ed.touchedPad);
        }
    };

    GLOBED_ASSERT(state.player1.has_value());

    if (state.player2) {
        auto dual = data.initDual();
        auto p1 = dual.initPlayer1();
        auto p2 = dual.initPlayer2();
        initPlayer(*state.player1, p1);
        initPlayer(*state.player2, p2);
    } else {
        auto single = data.initSingle();
        auto p1 = single.initPlayer1();
        initPlayer(*state.player1, p1);
    }
}

$implDecode(PlayerState, game::PlayerData::Reader& reader) {
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

        if (src.hasExtData()) {
            auto ed = src.getExtData();
            ExtendedPlayerData ext{};
            ext.velocityX = ed.getVelocityX();
            ext.velocityY = ed.getVelocityY();
            ext.accelerating = ed.getAccelerating();
            ext.acceleration = ed.getAcceleration();
            ext.fallStartY = ed.getFallStartY();
            ext.isOnGround2 = ed.getIsOnGround2();
            ext.gravityMod = ed.getGravityMod();
            ext.gravity = ed.getGravity();
            ext.touchedPad = ed.getTouchedPad();
            dst.extData = ext;
        }
    };

    if (reader.isDual()) {
        auto dual = reader.getDual();
        if (!dual.hasPlayer1() || !dual.hasPlayer2()) {
            return std::nullopt;
        }

        auto p1 = dual.getPlayer1();
        auto p2 = dual.getPlayer2();

        out.player1 = PlayerObjectData{};
        out.player2 = PlayerObjectData{};
        initPlayer(p1, *out.player1);
        initPlayer(p2, *out.player2);
    } else if (reader.isSingle()) {
        auto single = reader.getSingle();
        if (!single.hasPlayer1()) {
            return std::nullopt;
        }

        auto p1 = single.getPlayer1();
        out.player1 = PlayerObjectData{};
        out.player2 = std::nullopt;
        initPlayer(p1, *out.player1);
    } else if (reader.isCulled()) {
        // nothing
    } else {
        return std::nullopt;
    }

    return out;
}

// Icon data

$implEncode(const PlayerIconData& icons, shared::PlayerIconData::Builder& data) {
    data.setCube(icons.cube);
    data.setShip(icons.ship);
    data.setBall(icons.ball);
    data.setUfo(icons.ufo);
    data.setWave(icons.wave);
    data.setRobot(icons.robot);
    data.setSpider(icons.spider);
    data.setSwing(icons.swing);
    data.setJetpack(icons.jetpack);
    data.setColor1(icons.color1.inner());
    data.setColor2(icons.color2.inner());
    data.setGlowColor(icons.glowColor.inner());
    data.setDeathEffect(icons.deathEffect);
    data.setTrail(icons.trail);
    data.setShipTrail(icons.shipTrail);
}

$implDecode(PlayerIconData, shared::PlayerIconData::Reader& reader) {
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

$implEncode(const RoomSettings& settings, main::RoomSettings::Builder& data) {
    data.setServerId(settings.serverId);
    data.setPlayerLimit(settings.playerLimit);
    data.setFasterReset(settings.fasterReset);
    data.setHidden(settings.hidden);
    data.setPrivateInvites(settings.privateInvites);
    data.setIsFollower(settings.isFollower);
    data.setLevelIntegrity(settings.levelIntegrity);
    data.setTeams(settings.teams);
    data.setLockedTeams(settings.lockedTeams);
    data.setCollision(settings.collision);
    data.setTwoPlayerMode(settings.twoPlayerMode);
    data.setDeathlink(settings.deathlink);
}

$implDecode(RoomSettings, main::RoomSettings::Reader& reader) {
    RoomSettings out{};
    out.serverId = reader.getServerId();
    out.playerLimit = reader.getPlayerLimit();
    out.fasterReset = reader.getFasterReset();
    out.hidden = reader.getHidden();
    out.privateInvites = reader.getPrivateInvites();
    out.isFollower = reader.getIsFollower();
    out.levelIntegrity = reader.getLevelIntegrity();
    out.teams = reader.getTeams();
    out.lockedTeams = reader.getLockedTeams();
    out.collision = reader.getCollision();
    out.twoPlayerMode = reader.getTwoPlayerMode();
    out.deathlink = reader.getDeathlink();
    return out;
}

/// Multi color

inline std::optional<MultiColor> decodeMultiColor(capnp::Data::Reader data) {
    if (data.size() == 0) return std::nullopt;

    auto mc = MultiColor::decode({data.begin(), data.size()});
    if (!mc) {
        geode::log::warn(
            "Failed to decode multicolor '{}': {}",
            globed::hexEncode(data.begin(), data.size()),
            mc.unwrapErr()
        );

        return std::nullopt;
    } else {
        return *mc;
    }
}

// Display data

$implDecode(PlayerDisplayData, shared::PlayerDisplayData::Reader& reader) {
    PlayerDisplayData out{};
    out.accountId = reader.getAccountId();

    if (out.accountId == 0) {
        return std::nullopt;
    }

    out.userId = reader.getUserId();
    out.username = reader.getUsername();

    auto iconsR = reader.getIcons();

    if (auto icons = data::decodeOpt<PlayerIconData>(iconsR)) {
        out.icons = *icons;
    } else {
        return std::nullopt;
    }

    if (reader.hasSpecialData()) {
        SpecialUserData sud{};

        auto sd = reader.getSpecialData();

        if (sd.hasNameColor()) {
            sud.nameColor = decodeMultiColor(sd.getNameColor());
        }

        auto roles = sd.getRoles();
        sud.roleIds = {roles.begin(), roles.end()};

        if (!sud.roleIds.empty() || sud.nameColor) {
            out.specialUserData = std::move(sud);
        }
    }

    return out;
}

// Account data

$implDecode(PlayerAccountData, main::PlayerAccountData::Reader& reader) {
    PlayerAccountData out{};
    out.accountId = reader.getAccountId();
    out.userId = reader.getUserId();
    out.username = reader.getUsername();
    return out;
}

// Minimal room player

$implDecode(MinimalRoomPlayer, main::MinimalRoomPlayer::Reader& reader) {
    MinimalRoomPlayer out{};
    out.cube = reader.getCube();
    out.color1 = reader.getColor1();
    out.color2 = reader.getColor2();
    out.glowColor = reader.getGlowColor();
    out.accountData = data::decodeUnchecked<PlayerAccountData>(reader.getAccountData());
    return out;
}

$implDecode(MinimalRoomPlayer, main::RoomPlayer::Reader& reader) {
    MinimalRoomPlayer out{};
    out.cube = reader.getCube();
    out.color1 = reader.getColor1();
    out.color2 = reader.getColor2();
    out.glowColor = reader.getGlowColor();
    out.accountData = data::decodeUnchecked<PlayerAccountData>(reader.getAccountData());
    return out;
}

// Room player

$implDecode(RoomPlayer, main::RoomPlayer::Reader& reader) {
    RoomPlayer out{decodeUnchecked<MinimalRoomPlayer>(reader)};
    out.session = SessionId(reader.getSession());
    out.teamId = reader.getTeamId();

    if (reader.hasSpecialData()) {
        SpecialUserData sud{};
        auto rsud = reader.getSpecialData();

        if (rsud.hasNameColor()) {
            sud.nameColor = decodeMultiColor(rsud.getNameColor());
        }

        sud.roleIds = {rsud.getRoles().begin(), rsud.getRoles().end()};
        out.specialUserData = std::move(sud);
    }

    return out;
}

// Room state

$implDecode(msg::RoomStateMessage, main::RoomStateMessage::Reader& reader) {
    msg::RoomStateMessage out{};
    out.roomId = reader.getRoomId();
    out.roomOwner = reader.getRoomOwner();
    out.roomName = reader.getRoomName();
    out.passcode = reader.getPasscode();
    out.playerCount = reader.getPlayerCount();

    auto players = reader.getPlayers();
    out.players.reserve(players.size());

    for (auto player : players) {
        out.players.push_back(data::decodeUnchecked<RoomPlayer>(player));
    }

    out.settings = data::decodeUnchecked<RoomSettings>(reader.getSettings());

    auto teams = reader.getTeams();
    out.teams.reserve(teams.size());

    for (auto team : teams) {
        out.teams.push_back(RoomTeam {
            .color = decodeColor4(team),
        });
    }

    return out;
}

$implDecode(msg::RoomPlayersMessage, main::RoomPlayersMessage::Reader& reader) {
    msg::RoomPlayersMessage out{};

    auto players = reader.getPlayers();
    out.players.reserve(players.size());

    for (auto player : players) {
        out.players.push_back(data::decodeUnchecked<RoomPlayer>(player));
    }

    return out;
}

$implDecode(msg::RoomJoinFailedMessage, main::RoomJoinFailedMessage::Reader& reader) {
    using enum schema::main::RoomJoinFailedReason;

    msg::RoomJoinFailedMessage out{};
    out.reason = static_cast<msg::RoomJoinFailedReason>(reader.getReason());

    return out.reason <= msg::RoomJoinFailedReason::Last_ ? std::optional{out} : std::nullopt;
}

// Room create failed

$implDecode(msg::RoomCreateFailedMessage, main::RoomCreateFailedMessage::Reader& reader) {
    using enum schema::main::RoomCreateFailedReason;

    msg::RoomCreateFailedMessage out{};
    out.reason = static_cast<msg::RoomCreateFailedReason>(reader.getReason());

    return out.reason <= msg::RoomCreateFailedReason::Last_ ? std::optional{out} : std::nullopt;
}

// Room listing

$implDecode(msg::RoomListMessage, main::RoomListMessage::Reader& reader) {
    msg::RoomListMessage out{};
    auto rooms = reader.getRooms();
    out.rooms.reserve(rooms.size());

    for (auto room : rooms) {
        globed::RoomListingInfo info{};

        info.roomId = room.getRoomId();
        info.roomName = room.getRoomName();
        info.roomOwner = data::decodeUnchecked<RoomPlayer>(room.getRoomOwner());
        info.playerCount = room.getPlayerCount();
        info.hasPassword = room.getHasPassword();
        info.settings = data::decodeUnchecked<RoomSettings>(room.getSettings());

        out.rooms.push_back(std::move(info));
    }

    return std::optional{out};
}

// Level data

$implDecode(msg::LevelDataMessage, game::LevelDataMessage::Reader& reader) {
    auto players = reader.getPlayers();
    auto ddatas = reader.getDisplayDatas();
    auto eventData = reader.getEventData();

    msg::LevelDataMessage outMsg;
    outMsg.players.reserve(players.size());
    outMsg.displayDatas.reserve(ddatas.size());

    for (auto player : players) {
        if (auto s = data::decodeOpt<PlayerState>(player)) {
            outMsg.players.emplace_back(*std::move(s));
        } else if (player.getAccountId() != 0) {
            geode::log::warn("Server sent invalid player state data for {}, skipping", player.getAccountId());
        }
    }

    for (auto dd : ddatas) {
        if (auto s = data::decodeOpt<PlayerDisplayData>(dd)) {
            outMsg.displayDatas.push_back(*std::move(s));
        } else {
            // can happen as an optimization
            // geode::log::debug("Server sent invalid player display data, skipping");
        }
    }

    qn::ByteReader evReader{eventData.begin(), eventData.size()};

    while (evReader.remainingSize() > 0) {
        if (auto res = InEvent::decode(evReader)) {
            outMsg.events.push_back(std::move(*res));
        } else {
            geode::log::warn("failed to decode event: {}", res.unwrapErr());
            break;
        }
    }

    return outMsg;
}

/// Script logs

$implDecode(msg::ScriptLogsMessage, game::ScriptLogsMessage::Reader& reader) {
    std::vector<std::string> logs = {reader.getLogs().begin(), reader.getLogs().end()};

    return msg::ScriptLogsMessage { std::move(logs), reader.getRamUsage() };
}

/// Voice broadcast

$implDecode(msg::VoiceBroadcastMessage, game::VoiceBroadcastMessage::Reader& reader) {
    EncodedAudioFrame out{};
    int user = reader.getAccountId();

    for (auto frame : reader.getFrames()) {
        auto ptr = frame.begin();
        size_t size = frame.size();

        auto data = std::make_shared<uint8_t[]>(size);
        std::memcpy(data.get(), ptr, size);
        (void) out.pushOpusFrame({ .data = std::move(data), .size = size });
    }

    return msg::VoiceBroadcastMessage { .accountId = user, .frame = std::move(out) };
}

/// User role

inline std::optional<UserRole> decodeUserRole(const schema::shared::UserRole::Reader& reader, size_t idx) {
    UserRole role{};
    role.id = idx;
    role.stringId = reader.getStringId();
    role.icon = reader.getIcon();

    auto color = decodeMultiColor(reader.getNameColor());
    if (!color) {
        return std::nullopt;
    }

    role.nameColor = *color;
    return role;
}

static FeatureTier decodeFeatureTier(uint8_t tier) {
    return (FeatureTier)std::clamp<uint8_t>(tier, 0, 2);
}

$implDecode(msg::CentralLoginOkMessage, main::LoginOkMessage::Reader& reader) {
    msg::CentralLoginOkMessage msg{};

    if (reader.hasNewToken()) {
        msg.newToken = reader.getNewToken();
    }

    if (reader.hasAllRoles()) {
        auto roles = reader.getAllRoles();
        msg.allRoles.reserve(roles.size());

        size_t idx = 0;
        for (auto role : roles) {
            auto r = decodeUserRole(role, idx++);
            if (!r) {
                geode::log::warn("Failed to decode user role sent by central server");
                return std::nullopt;
            }

            msg.allRoles.push_back(std::move(*r));
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

    if (reader.hasNameColor()) {
        msg.nameColor = decodeMultiColor(reader.getNameColor());
    }

    msg.perms.isModerator = reader.getIsModerator();
    msg.perms.canMute = reader.getCanMute();
    msg.perms.canBan = reader.getCanBan();
    msg.perms.canSetPassword = reader.getCanSetPassword();
    msg.perms.canEditRoles = reader.getCanEditRoles();
    msg.perms.canSendFeatures = reader.getCanSendFeatures();
    msg.perms.canRateFeatures = reader.getCanRateFeatures();

    FeaturedLevelMeta flm{};
    flm.levelId = reader.getFeaturedLevel();
    flm.rateTier = decodeFeatureTier(reader.getFeaturedLevelTier());
    flm.edition = reader.getFeaturedLevelEdition();

    if (flm.levelId != 0) {
        msg.featuredLevel = flm;
    }

    return msg;
}

// Banned

$implDecode(msg::BannedMessage, main::BannedMessage::Reader& reader) {
    msg::BannedMessage out{};
    out.reason = reader.getReason();
    out.expiresAt = reader.getExpiresAt();
    return out;
}

// Muted

$implDecode(msg::MutedMessage, main::MutedMessage::Reader& reader) {
    msg::MutedMessage out{};
    out.reason = reader.getReason();
    out.expiresAt = reader.getExpiresAt();
    return out;
}

// Room banned

$implDecode(msg::RoomBannedMessage, main::RoomBannedMessage::Reader& reader) {
    msg::RoomBannedMessage out{};
    out.reason = reader.getReason();
    out.expiresAt = reader.getExpiresAt();
    return out;
}

/// Team creation result

$implDecode(msg::TeamCreationResultMessage, main::TeamCreationResultMessage::Reader& reader) {
    msg::TeamCreationResultMessage out{};

    out.success = reader.getSuccess();
    out.teamCount = reader.getTeamCount();

    return out;
}

/// Team changed

$implDecode(msg::TeamChangedMessage, main::TeamChangedMessage::Reader& reader) {
    msg::TeamChangedMessage out{};
    out.teamId = reader.getTeamId();
    return out;
}

/// Team members

$implDecode(msg::TeamMembersMessage, main::TeamMembersMessage::Reader& reader) {
    msg::TeamMembersMessage out{};

    auto members = reader.getMembers();
    auto teams = reader.getTeamIds();
    out.members.resize(std::min(members.size(), teams.size()));

    size_t i = 0;
    for (auto member : members) {
        out.members[i++].first = member;
    }

    i = 0;
    for (auto team : teams) {
        out.members[i++].second = team;
    }

    return out;
}

/// Teams updated message

$implDecode(msg::TeamsUpdatedMessage, main::TeamsUpdatedMessage::Reader& reader) {
    msg::TeamsUpdatedMessage out{};

    auto teams = reader.getTeams();
    out.teams.reserve(teams.size());

    for (auto team : teams) {
        out.teams.push_back(RoomTeam {
            .color = decodeColor4(team),
        });
    }

    return out;
}

/// UserDataChangedMessage

$implDecode(msg::UserDataChangedMessage, main::UserDataChangedMessage::Reader& reader) {
    msg::UserDataChangedMessage out{};

    out.roles = {reader.getRoles().begin(), reader.getRoles().end()};
    out.nameColor = decodeMultiColor(reader.getNameColor());
    out.perms.isModerator = reader.getIsModerator();
    out.perms.canMute = reader.getCanMute();
    out.perms.canBan = reader.getCanBan();
    out.perms.canSetPassword = reader.getCanSetPassword();
    out.perms.canEditRoles = reader.getCanEditRoles();
    out.perms.canSendFeatures = reader.getCanSendFeatures();
    out.perms.canRateFeatures = reader.getCanRateFeatures();

    return out;
}

/// Player counts message

$implDecode(msg::PlayerCountsMessage, main::PlayerCountsMessage::Reader& reader) {
    msg::PlayerCountsMessage out{};

    auto counts = reader.getCounts();
    auto levelIds = reader.getLevelIds();

    size_t count = std::min(counts.size(), levelIds.size());
    out.counts.reserve(count);

    for (size_t i = 0; i < count; i++) {
        out.counts.push_back({SessionId{levelIds[i]}, counts[i]});
    }

    return out;
}

/// Global players message

$implDecode(msg::GlobalPlayersMessage, main::GlobalPlayersMessage::Reader& reader) {
    msg::GlobalPlayersMessage out{};

    auto players = reader.getPlayers();
    out.players.reserve(players.size());

    for (auto pl : players) {
        out.players.push_back(decodeUnchecked<MinimalRoomPlayer>(pl));
    }

    return out;
}

/// Level list message

$implDecode(msg::LevelListMessage, main::LevelListMessage::Reader& reader) {
    msg::LevelListMessage out{};

    auto counts = reader.getPlayerCounts();
    auto levelIds = reader.getLevelIds();

    size_t count = std::min(counts.size(), levelIds.size());
    out.levels.reserve(count);

    for (size_t i = 0; i < count; i++) {
        out.levels.push_back({SessionId{levelIds[i]}, counts[i]});
    }

    return out;
}

/// Credits user

$implDecode(CreditsUser, main::CreditsUser::Reader& reader) {
    CreditsUser out{};

    out.accountId = reader.getAccountId();
    out.userId = reader.getUserId();
    out.username = reader.getUsername();
    out.displayName = reader.getDisplayName();
    out.cube = reader.getCube();
    out.color1 = reader.getColor1();
    out.color2 = reader.getColor2();
    out.glowColor = reader.getGlowColor();

    return out;
}

/// Credits

$implDecode(msg::CreditsMessage, main::CreditsMessage::Reader& reader) {
    msg::CreditsMessage out{};

    out.unavailable = reader.getUnavailable();

    if (out.unavailable) {
        return out;
    }

    auto cats = reader.getCategories();
    out.categories.reserve(cats.size());

    for (auto cat : cats) {
        auto users = cat.getUsers();

        CreditsCategory outc{};
        outc.name = cat.getName();
        outc.users.reserve(users.size());

        for (auto user : users) {
            outc.users.push_back(decodeUnchecked<CreditsUser>(user));
        }

        out.categories.push_back(std::move(outc));
    }

    return out;
}

/// Discord link stae

$implDecode(msg::DiscordLinkStateMessage, main::DiscordLinkStateMessage::Reader& reader) {
    msg::DiscordLinkStateMessage out{};
    out.id = reader.getId();
    out.username = reader.getUsername();
    out.avatarUrl = reader.getAvatarUrl();
    return out;
}

/// Discord link attempt

$implDecode(msg::DiscordLinkAttemptMessage, main::DiscordLinkAttemptMessage::Reader& reader) {
    msg::DiscordLinkAttemptMessage out{};
    out.id = reader.getId();
    out.username = reader.getUsername();
    out.avatarUrl = reader.getAvatarUrl();
    return out;
}

/// Featured level

$implDecode(msg::FeaturedLevelMessage, main::FeaturedLevelMessage::Reader& reader) {
    msg::FeaturedLevelMessage out{};
    FeaturedLevelMeta flm{};
    flm.levelId = reader.getLevelId();
    flm.rateTier = decodeFeatureTier(reader.getRateTier());
    flm.edition = reader.getEdition();

    if (flm.levelId != 0) {
        out.meta = flm;
    }

    return out;
}

/// Featured list

$implDecode(msg::FeaturedListMessage, main::FeaturedListMessage::Reader& reader) {
    msg::FeaturedListMessage out{};
    auto levelIds = reader.getLevelIds();
    auto rateTiers = reader.getRateTiers();

    for (size_t i = 0; i < std::min(levelIds.size(), rateTiers.size()); i++) {
        out.levels.push_back(FeaturedLevelMeta {
            .levelId = levelIds[i],
            .rateTier = decodeFeatureTier(rateTiers[i]),
        });
    }

    out.page = reader.getPage();
    out.totalPages = reader.getTotalPages();
    return out;
}

/// Invited

$implDecode(msg::InvitedMessage, main::InvitedMessage::Reader& reader) {
    msg::InvitedMessage out{};
    out.token = reader.getToken();

    auto invb = reader.getInvitedBy();
    out.invitedBy.accountId = invb.getAccountId();
    out.invitedBy.userId = invb.getUserId();
    out.invitedBy.username = invb.getUsername();

    return out;
}

/// Invite token created

$implDecode(msg::InviteTokenCreatedMessage, main::InviteTokenCreatedMessage::Reader& reader) {
    return msg::InviteTokenCreatedMessage {
        .token = reader.getToken()
    };
}

/// User punishment

$implDecode(UserPunishment, main::UserPunishment::Reader& reader) {
    UserPunishment out{};
    out.issuedBy = reader.getIssuedBy();
    out.issuedAt = reader.getIssuedAt();
    out.expiresAt = reader.getExpiresAt();
    out.reason = reader.getReason();
    return out;
}

/// Admin fetch response message

$implDecode(msg::AdminFetchResponseMessage, main::AdminFetchResponseMessage::Reader& reader) {
    msg::AdminFetchResponseMessage out{};

    out.accountId = reader.getAccountId();
    out.found = reader.getFound();
    out.whitelisted = reader.getWhitelisted();
    out.punishmentCount = reader.getPunishmentCount();
    out.roles = {reader.getRoles().begin(), reader.getRoles().end()};

    if (reader.hasActiveBan()) {
        out.activeBan = data::decodeUnchecked<UserPunishment>(reader.getActiveBan());
    }

    if (reader.hasActiveRoomBan()) {
        out.activeRoomBan = data::decodeUnchecked<UserPunishment>(reader.getActiveRoomBan());
    }

    if (reader.hasActiveMute()) {
        out.activeMute = data::decodeUnchecked<UserPunishment>(reader.getActiveMute());
    }

    return out;
}

/// Fetched mod

$implDecode(FetchedMod, main::FetchedMod::Reader& reader) {
    FetchedMod out{};
    out.accountId = reader.getAccountId();
    out.username = reader.getUsername();
    out.cube = reader.getCube();
    out.color1 = reader.getColor1();
    out.color2 = reader.getColor2();
    out.glowColor = reader.getGlowColor();
    return out;
}

/// Admin fetch mods response message

$implDecode(msg::AdminFetchModsResponseMessage, main::AdminFetchModsResponseMessage::Reader& reader) {
    msg::AdminFetchModsResponseMessage out{};

    auto users = reader.getUsers();
    out.users.reserve(users.size());

    for (auto user : users) {
        out.users.push_back(decodeUnchecked<FetchedMod>(user));
    }

    return out;
}

/// Admin audit log

$implDecode(AdminAuditLog, main::AuditLog::Reader& reader) {
    AdminAuditLog out{};

    out.id = reader.getId();
    out.accountId = reader.getAccountId();
    out.targetId = reader.getTargetAccountId();
    out.type = reader.getType();
    out.timestamp = reader.getTimestamp();
    out.expiresAt = reader.getExpiresAt();
    out.message = reader.getMessage();

    return out;
}

/// Admin fetch logs response message

$implDecode(msg::AdminLogsResponseMessage, main::AdminLogsResponseMessage::Reader& reader) {
    msg::AdminLogsResponseMessage out{};

    auto accs = reader.getAccounts();
    auto logs = reader.getLogs();

    out.users.reserve(accs.size());
    out.logs.reserve(logs.size());

    for (auto acc : accs) {
        out.users.push_back(decodeUnchecked<PlayerAccountData>(acc));
    }

    for (auto log : logs) {
        out.logs.push_back(decodeUnchecked<AdminAuditLog>(log));
    }

    return out;
}

// Admin punishment reasons message

$implDecode(msg::AdminPunishmentReasonsMessage, main::AdminPunishmentReasonsMessage::Reader& reader) {
    PunishReasons out{};

    auto banReasons = reader.getBan();
    out.banReasons = {banReasons.begin(), banReasons.end()};

    auto muteReasons = reader.getMute();
    out.muteReasons = {muteReasons.begin(), muteReasons.end()};

    auto roomBanReasons = reader.getRoomBan();
    out.roomBanReasons = {roomBanReasons.begin(), roomBanReasons.end()};

    return msg::AdminPunishmentReasonsMessage{ std::move(out) };
}

/// Send level script message

$implEncode(const std::vector<EmbeddedScript>& scripts, game::SendLevelScriptMessage::Builder& out) {
    auto outscr = out.initScripts(scripts.size());

    for (size_t i = 0; i < scripts.size(); i++) {
        auto& script = scripts[i];
        auto scr = outscr[i];
        scr.setContent(script.content);
        scr.setFilename(script.filename);
        scr.setMain(script.main);
        if (script.signature) {
            scr.setSignature({script.signature->data(), script.signature->size()});
        }
    }
}

}