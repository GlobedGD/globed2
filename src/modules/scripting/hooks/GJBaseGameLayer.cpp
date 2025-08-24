#include "GJBaseGameLayer.hpp"

#include <globed/core/RoomManager.hpp>
#include <globed/util/Random.hpp>
#include <core/net/NetworkManagerImpl.hpp>
#include <core/hooks/GJBaseGameLayer.hpp>
#include <modules/scripting/data/SpawnData.hpp>
#include <modules/scripting/data/SetItemData.hpp>
#include <modules/scripting/data/MoveGroupData.hpp>
#include <modules/scripting/data/FollowPlayerData.hpp>

using namespace geode::prelude;

namespace globed {

SCBaseGameLayer* SCBaseGameLayer::get(GJBaseGameLayer* base) {
    if (!base) base = GlobedGJBGL::get();

    return static_cast<SCBaseGameLayer*>(base);
}

void SCBaseGameLayer::postInit(const std::vector<EmbeddedScript>& scripts) {
    auto& nm = NetworkManagerImpl::get();

    m_fields->m_listener = nm.listen<msg::LevelDataMessage>([this](const auto& msg) {
        auto& fields = *m_fields.self();

        for (auto& event : msg.events) {
            this->handleEvent(event);
        }

        return ListenerResult::Continue;
    });

    m_fields->m_logsListener = nm.listen<msg::ScriptLogsMessage>([this](const auto& msg) {
        log::debug("Received {} logs from game server", msg.logs.size());

        for (auto& log : msg.logs) {
            m_fields->m_logBuffer.push_back(log);
        }

        return ListenerResult::Continue;
    });

    // send over the scripts to the server if we are the room owner
    auto& rm = RoomManager::get();
    if (rm.isOwner() && !scripts.empty()) {
        log::info("Sending {} scripts to the server", scripts.size());
        nm.sendLevelScript(scripts);

        auto gjbgl = GlobedGJBGL::get(this);
        gjbgl->customSchedule("2p-send-log-request", [this](GlobedGJBGL*, float dt) {
            this->sendLogRequest(dt);
        }, 1.0f);
    }

    this->schedule(schedule_selector(SCBaseGameLayer::processCustomFollowActions));
}

void SCBaseGameLayer::sendLogRequest(float) {
    Event ev{};
    ev.type = EVENT_SCR_REQUEST_SCRIPT_LOGS;
    NetworkManagerImpl::get().queueGameEvent(std::move(ev));
}

void SCBaseGameLayer::customMoveBy(int group, double dx, double dy) {
    this->moveObjectsSilent(group, dx, dy);
}

void SCBaseGameLayer::customMoveTo(int group, int center, double x, double y) {
    auto centerGroup = this->getGroup(center);
    if (!centerGroup || centerGroup->count() == 0) return;

    auto parent = this->getGroupParent(group);
    GameObject* centerObj;

    // random unless group id parent is present
    if (parent) {
        centerObj = parent;
    } else {
        centerObj = CCArrayExt<GameObject*>(centerGroup)[rng()->random<size_t>(0, centerGroup->count() - 1)];
    }

    double dx = x - centerObj->m_positionX;
    double dy = y - centerObj->m_positionY;

    this->customMoveBy(group, dx, dy);
}

void SCBaseGameLayer::customMoveDirection(
    int group,
    int targetPosGroup,
    int centerGroup,
    float distance,
    float time,
    bool silent,
    bool onlyX,
    bool onlyY,
    bool dynamic
) {
    // auto obj = new EffectGameObject;
    // obj->m_objectID = 901;
    // obj->m_targetGroupID = group;
    // obj->m_targetControlID = targetPosGroup;
    // obj->m_centerGroupID = centerGroup;
    // obj->m_directionModeDistance = distance;
    // obj->m_duration = time;
    // obj->m_isSilent = silent;
    // obj->m_isDynamicMode = dynamic;

    // this->triggerMoveCommand(obj);
}

void SCBaseGameLayer::customFollowPlayer(int id, int group, bool enable) {
    auto& fields = *m_fields.self();

    auto gjbgl = GlobedGJBGL::get(this);
    auto rp = gjbgl->getPlayer(id);
    if (!rp && enable) {
        log::warn("Tried to follow invalid player: {}", id);
        return;
    }

    if (enable) {
        fields.m_customFollowers.insert(std::make_pair(id, group));
        fields.m_lastPlayerPositions[id] = rp->player1()->getLastPosition();
    } else {
        fields.m_customFollowers.erase(std::make_pair(id, group));
        fields.m_lastPlayerPositions.erase(id);
    }
}

void SCBaseGameLayer::processCustomFollowActions(float) {
    auto& fields = *m_fields.self();
    auto gjbgl = GlobedGJBGL::get(this);

    std::vector<int> toUnfollow;

    for (auto& [player, group] : fields.m_customFollowers) {
        auto rp = gjbgl->getPlayer(player);
        if (!rp) {
            // unfollow
            toUnfollow.push_back(player);
            continue;
        }

        auto& lastPos = fields.m_lastPlayerPositions[player];
        auto pos = rp->player1()->getLastPosition();

        float dx = pos.x - lastPos.x;
        float dy = pos.y - lastPos.y;
        this->customMoveBy(group, dx, dy);
        lastPos = pos;
    }

    for (int id : toUnfollow) {
        this->unfollowAllForPlayer(id);
    }
}

void SCBaseGameLayer::unfollowAllForPlayer(int id) {
    // TODO: im too sleepy to do this rn
}

std::vector<std::string>& SCBaseGameLayer::getLogs() {
    return m_fields->m_logBuffer;
}

void SCBaseGameLayer::handleEvent(const Event& event) {
    auto& fields = *m_fields.self();
    qn::ByteReader reader{event.data};

    switch (event.type) {
        case EVENT_SCR_SPAWN_GROUP: {
            auto res = globed::decodeSpawnData(reader);
            if (!res) {
                log::warn("Failed to decode spawn group data: {}", res.unwrapErr());
                return;
            }

            auto data = std::move(res).unwrap();
            double delay = data.delay.value_or(0.f);

            if (data.delayVariance > 0.f) {
                double min = std::max(0.0, delay - data.delayVariance);
                double max = delay + data.delayVariance;
                delay = rng()->random(min, max);
            }

            log::debug("Spawning group {} with delay {} (ordered: {})", data.groupId, delay, data.ordered);

            this->spawnGroup(
                data.groupId,
                data.ordered,
                delay,
                data.remaps,
                0,
                0
            );
        } break;

        case EVENT_SCR_SET_ITEM: {
            auto res = globed::decodeSetItemData(reader);
            if (!res) {
                log::warn("Failed to decode set item data: {}", res.unwrapErr());
                return;
            }

            auto data = std::move(res).unwrap();
            m_effectManager->updateCountForItem(data.itemId, data.value);
            this->updateCounters(data.itemId, data.value);
        } break;

        case EVENT_SCR_MOVE_GROUP: {
            auto res = globed::decodeMoveGroupData(reader);
            if (!res) {
                log::warn("Failed to decode move group data: {}", res.unwrapErr());
                return;
            }

            auto data = std::move(res).unwrap();
            this->customMoveBy(data.group, data.x, data.y);
        } break;

        case EVENT_SCR_MOVE_GROUP_ABSOLUTE: {
            auto res = globed::decodeMoveAbsGroupData(reader);
            if (!res) {
                log::warn("Failed to decode move abs group data: {}", res.unwrapErr());
                return;
            }

            auto data = std::move(res).unwrap();
            this->customMoveTo(data.group, data.center, data.x, data.y);
        } break;

        case EVENT_SCR_FOLLOW_PLAYER: {
            auto res = globed::decodeFollowPlayerData(reader);
            if (!res) {
                log::warn("Failed to decode follow data: {}", res.unwrapErr());
                return;
            }

            auto data = std::move(res).unwrap();
            this->customFollowPlayer(data.player, data.group, data.enable);
        } break;
    }

    // auto it = fields.m_customListeners.find(event.type);
    // if (it == fields.m_customListeners.end()) {
    //     return;
    // }

    // for (auto groupId : it->second) {
    //     // TODO: figure out the arguments to pass
    //     this->spawnGroup(groupId, true, 0.0, {}, 0, 0);
    // }
}

void SCBaseGameLayer::addEventListener(const ListenEventPayload& obj) {
    // m_fields->m_customListeners[obj.eventId].push_back(obj.groupId);
}

}
