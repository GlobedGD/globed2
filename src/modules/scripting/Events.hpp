#pragma once
#include <globed/core/Event.hpp>
#include "data/SpawnData.hpp"
#include "data/SetItemData.hpp"
#include "data/FollowPlayerData.hpp"
#include "data/MoveGroupData.hpp"

namespace globed {

struct SpawnGroupEvent : ServerEvent<SpawnGroupEvent, EventServer::Game> {
    static constexpr auto Id = "globed/scripting.spawn-group";

    SpawnGroupEvent(SpawnData data) : data(std::move(data)) {}
    SpawnData data;

    static Result<SpawnGroupEvent> decode(std::span<const uint8_t> data);
};

struct SetItemEvent : ServerEvent<SetItemEvent, EventServer::Game> {
    static constexpr auto Id = "globed/scripting.set-item";

    SetItemEvent(SetItemData data) : data(std::move(data)) {}
    SetItemData data;

    static Result<SetItemEvent> decode(std::span<const uint8_t> data);
};

struct MoveGroupEvent : ServerEvent<MoveGroupEvent, EventServer::Game> {
    static constexpr auto Id = "globed/scripting.move-group";

    MoveGroupEvent(MoveGroupData data) : data(std::move(data)) {}
    MoveGroupData data;

    static Result<MoveGroupEvent> decode(std::span<const uint8_t> data);
};

struct FollowPlayerEvent : ServerEvent<FollowPlayerEvent, EventServer::Game> {
    static constexpr auto Id = "globed/scripting.follow-player";

    FollowPlayerEvent(FollowPlayerData data) : data(std::move(data)) {}
    FollowPlayerData data;

    static Result<FollowPlayerEvent> decode(std::span<const uint8_t> data);
};

struct FollowAbsoluteEvent : ServerEvent<FollowAbsoluteEvent, EventServer::Game> {
    static constexpr auto Id = "globed/scripting.follow-absolute";

    FollowAbsoluteEvent(FollowAbsoluteData data) : data(std::move(data)) {}
    FollowAbsoluteData data;

    static Result<FollowAbsoluteEvent> decode(std::span<const uint8_t> data);
};

struct FollowRotationEvent : ServerEvent<FollowRotationEvent, EventServer::Game> {
    static constexpr auto Id = "globed/scripting.follow-rotation";

    FollowRotationEvent(FollowRotationData data) : data(std::move(data)) {}

    FollowRotationData data;

    static Result<FollowRotationEvent> decode(std::span<const uint8_t> data);
};

struct ScriptedEvent : ServerEvent<ScriptedEvent, EventServer::Game> {
    static constexpr auto Id = "globed/scripting.custom";

    ScriptedEvent() = default;

    uint16_t type;
    std::vector<std::variant<int, float>> args;

    std::vector<uint8_t> encode() const;
};

struct RequestScriptLogsEvent : ServerEvent<RequestScriptLogsEvent, EventServer::Game> {
    static constexpr auto Id = "globed/scripting.request-script-logs";

    RequestScriptLogsEvent() = default;

    std::vector<uint8_t> encode() const { return {}; }
};

}
