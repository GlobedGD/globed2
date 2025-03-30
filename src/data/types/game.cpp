#include "game.hpp"

#include <data/bitbuffer.hpp>

using namespace cocos2d;

template<>
void ByteBuffer::customEncode<GlobedCounterChange>(const GlobedCounterChange& val) {
    using enum GlobedCounterChange::Type;

    this->writeValue(val.itemId);
    this->writeEnum(val.type);

    if (val.type == Add || val.type == Set) {
        this->writeI32(val._val.intVal);
    } else {
        this->writeF32(val._val.floatVal);
    }
}

template <>
ByteBuffer::DecodeResult<GlobedCounterChange> ByteBuffer::customDecode<GlobedCounterChange>() {
    using enum GlobedCounterChange::Type;

    GlobedCounterChange val;

    GLOBED_UNWRAP_INTO(this->readU16(), val.itemId);
    GLOBED_UNWRAP_INTO(this->readEnum<GlobedCounterChange::Type>(), val.type);

    if (val.type == Add || val.type == Set) {
        GLOBED_UNWRAP_INTO(this->readI32(), val._val.intVal);
    } else {
        GLOBED_UNWRAP_INTO(this->readF32(), val._val.floatVal);
    }

    return Ok(val);
}

void SpecificIconData::copyFlagsFrom(const SpecificIconData& other) {
    iconType = other.iconType;
    isDashing = other.isDashing;
    isLookingLeft = other.isLookingLeft;
    isUpsideDown = other.isUpsideDown;
    isVisible = other.isVisible;
    isMini = other.isMini;
    isGrounded = other.isGrounded;
    isStationary = other.isStationary;
    isFalling = other.isFalling;
    didJustJump = other.didJustJump;
    isRotating = other.isRotating;
    isSideways = other.isSideways;
}

template<> void ByteBuffer::customEncode(const SpecificIconData& data) {
    this->writeValue(data.position);
    this->writeValue(data.rotation);
    this->writeValue(data.iconType);

    BitBuffer<16> bits;
    bits.writeBits(
        data.isVisible,
        data.isLookingLeft,
        data.isUpsideDown,
        data.isDashing,
        data.isMini,
        data.isGrounded,
        data.isStationary,
        data.isFalling,
        data.didJustJump,
        data.isRotating,
        data.isSideways
    );
    this->writeBits(bits);

    this->writeValue(data.spiderTeleportData);
}

template<> ByteBuffer::DecodeResult<SpecificIconData> ByteBuffer::customDecode() {
    SpecificIconData data;

    GLOBED_UNWRAP_INTO(this->readValue<CCPoint>(), data.position);
    GLOBED_UNWRAP_INTO(this->readValue<float>(), data.rotation);
    GLOBED_UNWRAP_INTO(this->readValue<PlayerIconType>(), data.iconType);

    GLOBED_UNWRAP_INTO(this->readBits<16>(), auto bits);
    bits.readBitsInto(
        data.isVisible,
        data.isLookingLeft,
        data.isUpsideDown,
        data.isDashing,
        data.isMini,
        data.isGrounded,
        data.isStationary,
        data.isFalling,
        data.didJustJump,
        data.isRotating,
        data.isSideways
    );

    GLOBED_UNWRAP_INTO(this->readValue<std::optional<SpiderTeleportData>>(), data.spiderTeleportData);

    return Ok(data);
}

template<> void ByteBuffer::customEncode(const PlayerData& data) {
    this->writeValue(data.timestamp);
    this->writeValue(data.player1);
    this->writeValue(data.player2);
    this->writeValue(data.deathCounter);
    this->writeValue(data.currentPercentage);

    BitBuffer<8> bits;
    bits.writeBits(data.isDead, data.isPaused, data.isPracticing, data.isDualMode, data.isInEditor, data.isEditorBuilding, data.isLastDeathReal);
    this->writeBits(bits);
}

template<> ByteBuffer::DecodeResult<PlayerData> ByteBuffer::customDecode() {
    PlayerData data;

    GLOBED_UNWRAP_INTO(this->readValue<float>(), data.timestamp);
    GLOBED_UNWRAP_INTO(this->readValue<SpecificIconData>(), data.player1);
    GLOBED_UNWRAP_INTO(this->readValue<SpecificIconData>(), data.player2);
    GLOBED_UNWRAP_INTO(this->readValue<float>(), data.deathCounter);
    GLOBED_UNWRAP_INTO(this->readValue<float>(), data.currentPercentage);

    GLOBED_UNWRAP_INTO(this->readBits<8>(), auto bits);
    bits.readBitsInto(data.isDead, data.isPaused, data.isPracticing, data.isDualMode, data.isInEditor, data.isEditorBuilding, data.isLastDeathReal);

    return Ok(data);
}
