#include "GJBaseGameLayer.hpp"

#include <globed/core/RoomManager.hpp>
#include <Geode/utils/random.hpp>
#include <globed/util/algo.hpp>
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

    auto gjbgl = GlobedGJBGL::get(this);
    gjbgl->setDisallowThrottleUpdates();

    fields.m_listener = nm.listen<msg::LevelDataMessage>([this](const auto& msg) {
        auto& fields = *m_fields.self();

        for (auto& event : msg.events) {
            this->handleEvent(event);
        }
    });

    fields.m_logsListener = nm.listen<msg::ScriptLogsMessage>([this](const auto& msg) {
        auto& fields = *m_fields.self();

        for (auto& log : msg.logs) {
            log::debug("(Script) {}", log);
            fields.m_logBuffer.push_back(log);
        }

        fields.m_memLimitBuffer.push_back({ SystemTime::now(), msg.memUsage });
        if (fields.m_memLimitBuffer.size() > 300) {
            fields.m_memLimitBuffer.pop_front();
        }
    });

    // send over the scripts to the server if we are the room owner
    auto& rm = RoomManager::get();
    fields.m_hasScripts = !scripts.empty();

    if (rm.isOwner() && !scripts.empty()) {
        log::info("Sending {} scripts to the server", scripts.size());
        nm.queueLevelScript(scripts);
        fields.m_scriptsSent = true;

        this->schedule(schedule_selector(SCBaseGameLayer::sendLogRequest), 1.0f);
    }

    fields.m_localId = singleton<GJAccountManager>()->m_accountID;

    this->schedule(schedule_selector(SCBaseGameLayer::processCustomActions));
}

void SCBaseGameLayer::sendLogRequest(float) {
    NetworkManagerImpl::get().queueGameEvent(RequestScriptLogsEvent{});
}

void SCBaseGameLayer::customMoveBy(int group, double dx, double dy, float duration) {
    if (duration <= 0.f) {
        this->moveObjectsSilent(group, dx, dy);
        return;
    }

    // uhh
    m_fields->m_moveActions.emplace_back(CustomMoveAction {
        .m_groupId = group,
        .m_durationRem = duration,
        .m_dx = dx / duration,
        .m_dy = dy / duration,
    });
}

void SCBaseGameLayer::customMoveTo(int group, int center, double x, double y, float duration) {
    auto centerGroup = this->getGroup(center);
    if (!centerGroup || centerGroup->count() == 0) return;

    auto parent = this->getGroupParent(group);
    GameObject* centerObj;

    // random unless group id parent is present
    if (parent) {
        centerObj = parent;
    } else {
        centerObj = CCArrayExt<GameObject*>(centerGroup)[utils::random::generate<size_t>(0, centerGroup->count() - 1)];
    }

    double dx = x - centerObj->m_positionX;
    double dy = y - centerObj->m_positionY;

    this->customMoveBy(group, dx, dy, duration);
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

void SCBaseGameLayer::customRotateBy(int group, int center, double theta) {
    auto& fields = *m_fields.self();
    auto gjbgl = GlobedGJBGL::get(this);

    if (center == 0) {
        center = group;
    }

    auto centerObj = this->tryGetObject(center); // m_targetGroupID
    auto targetObj = this->tryGetObject(group);  // m_centerGroupID
    auto targetIdk = this->tryGetObject(group);  // m_rotationTargetID

    if (centerObj && targetObj && targetIdk) {
        // auto a = getRotateCommand
        // getRotateCommandAngleDelta
    }

    // log::debug("rotating this bitch!! {} against {} by {}", group, center, theta);

    m_effectManager->createRotateCommand(
        theta,
        0.f,
        group,
        center,
        (int)EasingType::None,
        0.f, // easing rate
        false, // m_lockObjectRotation
        false, false, // idk
        0, // m_uniqueID
        0 // m_controlID
    );
}

void SCBaseGameLayer::customFollowPlayerMov(int player, int group, bool enable) {
    auto& fields = *m_fields.self();
    auto gjbgl = GlobedGJBGL::get(this);

    if (!enable) {
        this->disableCustomFollow(player, group, true, false);
        return;
    }

    auto& action = this->insertCustomFollow(player, group);
    action.m_pos = true;
}

void SCBaseGameLayer::customFollowPlayerRot(int player, int group, int center, bool enable) {
    auto& fields = *m_fields.self();
    auto gjbgl = GlobedGJBGL::get(this);

    log::debug("follow rot {} {} {} {}", player, group, center, enable);

    if (!enable) {
        this->disableCustomFollow(player, group, false, true);
        return;
    }

    auto& action = this->insertCustomFollow(player, group);
    action.m_rot = true;
    action.m_centerGroupId = center;
}

CustomFollowAction& SCBaseGameLayer::insertCustomFollow(int player, int group) {
    auto& fields = *m_fields.self();
    auto gjbgl = GlobedGJBGL::get(this);

    size_t idx = this->getCustomFollowIndex(player, group);

    if (idx == (size_t)-1) {
        fields.m_followActions.push_back(CustomFollowAction {
            .m_playerId = player,
            .m_groupId = group,
        });
        idx = fields.m_followActions.size() - 1;
    }

    if (!fields.m_lastPlayerPositions.contains(player)) {
        auto pos = this->positionForPlayer(player);

        if (pos.pos.isZero()) {
            log::warn("Failed to locate player {}, unable to follow!", player);
        } else {
            fields.m_lastPlayerPositions[player] = pos;
        }
    }

    return fields.m_followActions[idx];
}

void SCBaseGameLayer::disableCustomFollow(int player, int group, bool disableMov, bool disableRot) {
    auto& fields = *m_fields.self();
    auto gjbgl = GlobedGJBGL::get(this);

    size_t idx = this->getCustomFollowIndex(player, group);

    if (idx == (size_t)-1) return;

    auto& action = fields.m_followActions[idx];
    if (disableMov) {
        action.m_pos = false;
    }

    if (disableRot) {
        action.m_rot = false;
    }

    if (!action.m_rot && !action.m_pos) {
        fields.m_followActions.erase(fields.m_followActions.begin() + idx);
    }
}

size_t SCBaseGameLayer::getCustomFollowIndex(int player, int group) {
    auto& fields = *m_fields.self();

    for (size_t i = 0; i < fields.m_followActions.size(); i++) {
        auto& act = fields.m_followActions[i];
        if (act.m_playerId == player && act.m_groupId == group) {
            return i;
        }
    }

    return -1;
}

CustomFollowedData SCBaseGameLayer::positionForPlayer(int player) {
    auto& fields = *m_fields.self();

    if (player == fields.m_localId) {
        return {
            m_player1->getPosition(),
            globed::normalizeAngle(m_player1->getRotation()),
        };
    } else {
        auto rp = GlobedGJBGL::get(this)->getPlayer(player);

        if (!rp) {
            return {};
        }

        return {
            rp->player1()->getLastPosition(),
            globed::normalizeAngle(rp->player1()->getLastRotation())
        };
    }
}

void SCBaseGameLayer::processCustomActions(float dt) {
    this->processCustomFollowActions(dt);
    this->processCustomMoveActions(dt);
}

void SCBaseGameLayer::processCustomFollowActions(float) {
    auto& fields = *m_fields.self();
    auto gjbgl = GlobedGJBGL::get(this);

    for (size_t i = 0; i < fields.m_followActions.size(); i++) {
        auto& action = fields.m_followActions[i];

        auto data = this->positionForPlayer(action.m_playerId);

        if (data.pos.isZero()) {
            continue;
        }

        auto it = fields.m_lastPlayerPositions.find(action.m_playerId);

        if (it == fields.m_lastPlayerPositions.end()) {
            // this player was not available when the follow action started, if they are now, then start following next frame
            fields.m_lastPlayerPositions.insert(it, std::make_pair(action.m_playerId, data));
            continue;
        }

        auto& lastData = it->second;

        if (action.m_pos) {
            float dx = data.pos.x - lastData.pos.x;
            float dy = data.pos.y - lastData.pos.y;
            this->customMoveBy(action.m_groupId, dx, dy);
        }

        if (action.m_rot) {
            float dr = globed::normalizeAngle(data.rot - lastData.rot);
            this->customRotateBy(action.m_groupId, action.m_centerGroupId, dr);
        }
    }

    for (auto& it : fields.m_lastPlayerPositions) {
        auto pos = this->positionForPlayer(it.first);
        it.second = pos;
    }
}

void SCBaseGameLayer::processCustomMoveActions(float dt) {
    auto& fields = *m_fields.self();
    auto gjbgl = GlobedGJBGL::get(this);

    for (size_t i = 0; i < fields.m_moveActions.size();) {
        auto& action = fields.m_moveActions[i];

        float mdt = std::clamp(dt, 0.0f, action.m_durationRem);
        action.m_durationRem -= mdt;

        float dx = action.m_dx * mdt;
        float dy = action.m_dy * mdt;
        this->customMoveBy(action.m_groupId, dx, dy);

        if (action.m_durationRem <= 0.f) {
            fields.m_moveActions.erase(fields.m_moveActions.begin() + i);
        } else {
            i++;
        }
    }
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
            delay = utils::random::generate(min, max);
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
        if (data.absolute) {
            this->customMoveTo(data.group, data.center, data.x, data.y, data.duration);
        } else {
            this->customMoveBy(data.group, data.x, data.y, data.duration);
        }
    } else if (event.is<FollowPlayerEvent>()) {
        auto& data = event.as<FollowPlayerEvent>().data;

        this->customFollowPlayerMov(data.player, data.group, data.enable);
    } else if (event.is<FollowRotationEvent>()) {
        auto& data = event.as<FollowRotationEvent>().data;

        this->customFollowPlayerRot(data.player, data.group, data.center, data.enable);
    }
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
