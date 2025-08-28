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
using namespace asp::time;

namespace globed {

SCBaseGameLayer* SCBaseGameLayer::get(GJBaseGameLayer* base) {
    if (!base) base = GlobedGJBGL::get();

    return static_cast<SCBaseGameLayer*>(base);
}

void SCBaseGameLayer::postInit(const std::vector<EmbeddedScript>& scripts) {
    auto& nm = NetworkManagerImpl::get();
    auto& fields = *m_fields.self();

    fields.m_listener = nm.listen<msg::LevelDataMessage>([this](const auto& msg) {
        auto& fields = *m_fields.self();

        for (auto& event : msg.events) {
            this->handleEvent(event);
        }

        return ListenerResult::Continue;
    });

    fields.m_logsListener = nm.listen<msg::ScriptLogsMessage>([this](const auto& msg) {
        auto& fields = *m_fields.self();

        log::debug("Received {} logs from game server, mem usage: {}", msg.logs.size(), msg.memUsage * 100.f);

        for (auto& log : msg.logs) {
            log::debug("(Script) {}", log);
            fields.m_logBuffer.push_back(log);
        }

        fields.m_memLimitBuffer.push_back({ SystemTime::now(), msg.memUsage });
        if (fields.m_memLimitBuffer.size() > 300) {
            fields.m_memLimitBuffer.pop_front();
        }

        return ListenerResult::Continue;
    });

    // send over the scripts to the server if we are the room owner
    auto& rm = RoomManager::get();
    if (rm.isOwner() && !scripts.empty()) {
        log::info("Sending {} scripts to the server", scripts.size());
        nm.queueLevelScript(scripts);

        auto gjbgl = GlobedGJBGL::get(this);
        gjbgl->customSchedule("2p-send-log-request", [this](GlobedGJBGL*, float dt) {
            this->sendLogRequest(dt);
        }, 1.0f);
    }

    fields.m_localId = cachedSingleton<GJAccountManager>()->m_accountID;

    this->schedule(schedule_selector(SCBaseGameLayer::processCustomFollowActions));
}

void SCBaseGameLayer::sendLogRequest(float) {
    NetworkManagerImpl::get().queueGameEvent(RequestScriptLogsEvent{});
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

    if (enable) {
        CCPoint curPos;
        if (id == fields.m_localId) {
            curPos = m_player1->getPosition();
        } else {
            auto rp = gjbgl->getPlayer(id);

            if (!rp) {
                log::warn("Tried to follow invalid player: {}", id);
                return;
            }

            curPos = rp->player1()->getLastPosition();
        }

        log::debug("Following player {} for group {}", id, group);
        fields.m_customFollowers.insert(std::make_pair(id, group));
        fields.m_lastPlayerPositions[id] = curPos;
    } else {
        log::debug("Unfollowing player {} for group {}", id, group);
        fields.m_customFollowers.erase(std::make_pair(id, group));

        // TODO: only remove if player is gone from the map
        // fields.m_lastPlayerPositions.erase(id);
    }
}

void SCBaseGameLayer::processCustomFollowActions(float) {
    auto& fields = *m_fields.self();
    auto gjbgl = GlobedGJBGL::get(this);

    std::vector<int> toUnfollow;

    for (auto& [player, group] : fields.m_customFollowers) {
        CCPoint pos;

        if (player == fields.m_localId) {
            pos = m_player1->getPosition();
        } else {
            auto rp = gjbgl->getPlayer(player);
            if (!rp) {
                // unfollow
                toUnfollow.push_back(player);
                continue;
            }

            pos = rp->player1()->getLastPosition();
        }

        auto& lastPos = fields.m_lastPlayerPositions[player];

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

std::deque<std::pair<asp::time::SystemTime, float>>& SCBaseGameLayer::getMemLimitBuffer() {
    return m_fields->m_memLimitBuffer;
}

void SCBaseGameLayer::handleEvent(const InEvent& event) {
    auto& fields = *m_fields.self();

    if (event.is<SpawnGroupEvent>()) {
        auto& data = event.as<SpawnGroupEvent>().data;

        double delay = data.delay.value_or(0.f);

        if (data.delayVariance > 0.f) {
            double min = std::max(0.0, delay - data.delayVariance);
            double max = delay + data.delayVariance;
            delay = rng()->random(min, max);
        }

        log::debug("Spawning group {} with delay {} (ordered: {}), remaps {}", data.groupId, delay, data.ordered, data.remaps.size());

        if (!data.remaps.empty()) {
            int id = m_spawnRemapTriggers.size();
            m_spawnRemapTriggers.push_back(std::unordered_map<int, int>{});

            for (size_t i = 0; i < data.remaps.size(); i += 2) {
                m_spawnRemapTriggers[id][data.remaps[i]] = data.remaps[i + 1];
            }

            this->spawnGroup(data.groupId, data.ordered, delay, {id, 0}, 0, 0);

            m_spawnRemapTriggers.erase(m_spawnRemapTriggers.begin() + id);
        } else {
            this->spawnGroup(data.groupId, data.ordered, delay, {}, 0, 0);
        }

    } else if (event.is<SetItemEvent>()) {
        auto& data = event.as<SetItemEvent>().data;

        m_effectManager->updateCountForItem(data.itemId, data.value);
        this->updateCounters(data.itemId, data.value);
    } else if (event.is<MoveGroupEvent>()) {
        auto& data = event.as<MoveGroupEvent>().data;

        this->customMoveBy(data.group, data.x, data.y);
    } else if (event.is<MoveGroupAbsoluteEvent>()) {
        auto& data = event.as<MoveGroupAbsoluteEvent>().data;

        this->customMoveTo(data.group, data.center, data.x, data.y);
    } else if (event.is<FollowPlayerEvent>()) {
        auto& data = event.as<FollowPlayerEvent>().data;

        this->customFollowPlayer(data.player, data.group, data.enable);
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

// class $modify(GJBaseGameLayer) {
//     void spawnGroup(int group, bool ordered, double delay, gd::vector<int> const& remapKeys, int triggerID, int controlID) {
//         log::debug("spawnGroup({}, {}, {}, {}, {}, {})", group, ordered, delay, remapKeys, triggerID, controlID);
//         log::debug("{}", m_spawnRemapTriggers);
//         GJBaseGameLayer::spawnGroup(group, ordered, delay, remapKeys, triggerID, controlID);
//     }
// };
