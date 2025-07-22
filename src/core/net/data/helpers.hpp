#pragma once

#include "generated.hpp"
#include <globed/core/game/PlayerState.hpp>

namespace globed::data {

inline void encodePlayerState(const PlayerState& state, auto& data) {
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

inline std::optional<PlayerState> decodePlayerState(schema::game::PlayerData::Reader& reader) {
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

    auto initPlayer = [](auto& src, PlayerObjectData& dst) {
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

}