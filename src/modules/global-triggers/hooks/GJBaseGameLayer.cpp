#include "GJBaseGameLayer.hpp"
#include "GJEffectManager.hpp"
#include "../Ids.hpp"

#include <core/net/NetworkManagerImpl.hpp>

#include <asp/time/SystemTime.hpp>

using namespace geode::prelude;
using namespace asp::time;

namespace globed {
#define $rid(name, offset) static constexpr inline int ITEM_##name = 80000 + offset
    $rid(ACCOUNT_ID, 0);
    $rid(LAST_JOINED, 1);
    $rid(LAST_LEFT, 2);
    $rid(TOTAL_PLAYERS, 3);
    $rid(TOTAL_PLAYERS_JOINED, 4);
    $rid(TOTAL_PLAYERS_LEFT, 5);
    $rid(INITIALIZED, 6);
    $rid(LOCAL_PING, 7);
    $rid(IS_EDITOR, 8);
    $rid(UNIX_TIME, 100);
    $rid(UNIX_TIME_SUBMS, 101);
    $rid(GLEPOCH_TIME, 102);
    $rid(GLEPOCH_TIME_SUBMS, 103);
    $rid(LAST_PAUSE_PLAYER, 200);
    $rid(LAST_PAUSE_TIMESTAMP, 201);
    $rid(LAST_PAUSE_TIMESTAMP_MS, 202);
    $rid(LEVEL_ID, 500);
#undef $rid

GTriggersGJBGL* GTriggersGJBGL::get(GJBaseGameLayer* base) {
    if (!base) base = GlobedGJBGL::get();

    return static_cast<GTriggersGJBGL*>(base);
}

static float checkedDiv(float a, float b) {
    if (b == 0.0f) {
        return 0.0f;
    }

    return a / b;
}

std::optional<int> GTriggersGJBGL::activateItemEditTriggerReimpl(ItemTriggerGameObject* obj) {
    int item1Id = obj->m_itemID;
    int item1Mode = obj->m_item1Mode;
    int item2Id = obj->m_itemID2;
    int item2Mode = obj->m_item2Mode;
    int resultType2 = std::max(obj->m_resultType2, 1);
    int resultType3 = std::max(obj->m_resultType3, 3);
    int targetMode = std::max(obj->m_targetItemMode, 1);
    float mod1 = std::round(obj->m_mod1 * 1000.0f) / 1000.f;

    auto applyOperation = [](double a, double b, int op) -> double {
        switch (op) {
            case 1: return a + b;
            case 2: return a - b;
            case 3: return a * b;
            case 4: return checkedDiv(a, b);
            default: return 0.0;
        }
    };

    auto applyRounding = [](double val, int roundType) -> double {
        switch (roundType) {
            case 1: return std::round(val);
            case 2: return std::floor(val);
            case 3: return std::ceil(val);
            default: return val;
        }
    };

    auto applySign = [](double val, int signType) -> double {
        switch (signType) {
            case 1: return std::abs(val);
            case 2: return -std::abs(val);
            default: return val;
        }
    };

    if (obj->m_targetGroupID > 0 || targetMode == 3) {
        bool bVar14 = item1Mode != 0 && (item1Id >= 1 || item1Mode <= 4);
        bool bVar13 = item2Mode != 0 && (item2Id >= 1 || item2Mode <= 4);

        if (!bVar14 && bVar13) {
            bVar14 = true;
            bVar13 = false;
            item1Mode = item2Mode;
            item1Id = item2Id;
        }

        double value1 = this->getItemValue(item1Mode, item1Id);
        double value2 = this->getItemValue(item2Mode, item2Id);
        double targetValue = this->getItemValue(targetMode, obj->m_targetGroupID);

        if (bVar13) {
            value1 = applyOperation(value1, value2, resultType2);
        }

        double result = mod1;
        if (bVar14) {
            result = applyOperation(value1, result, resultType3);
        }

        result = applyRounding(result, obj->m_roundType1);
        result = applySign(result, obj->m_signType1);
        if (result != 0) result = applyOperation(targetValue, result, obj->m_resultType1);
        result = applyRounding(result, obj->m_roundType2);
        result = applySign(result, obj->m_signType2);

        if (targetMode == 1) {
            return result;
        } else if (targetMode == 2) {
            m_effectManager->updateTimer(obj->m_targetGroupID, result);
        } else if (targetMode == 3) {
            m_gameState.m_unkBool32 = true;
            m_gameState.m_points = result;
        }

    }

    return std::nullopt;
}

void GTriggersGJBGL::activateItemEditTrigger(ItemTriggerGameObject* obj) {
    int targetId = obj->m_targetGroupID;

    if (!globed::isCustomItem(targetId)) {
        GJBaseGameLayer::activateItemEditTrigger(obj);
        return;
    }

    if (globed::isReadonlyCustomItem(targetId)) {
        return; // do nothing
    }

    auto efm = static_cast<HookedGJEffectManager*>(m_effectManager);

    int oldValue = efm->countForCustomItem(targetId);
    auto newValue = this->activateItemEditTriggerReimpl(obj);

    if (!newValue) {
        log::warn("Item edit trigger returned null, not updating (id {}, prev val {})", targetId, oldValue);
        return;
    }

    CounterChange cc{(uint32_t)targetId, (int32_t)*newValue, CounterChangeType::Set};
    GlobalTriggersModule::get().queueCounterChange(cc);
}

double GTriggersGJBGL::getItemValue(int a, int b) {
    if (a == 1 && globed::isCustomItem(b)) {
        auto value = static_cast<HookedGJEffectManager*>(m_effectManager)->countForCustomItem(b);
        // log::debug("getItemValue({}, {}) = {}", a, b, value);
        return value;
    }

    double x = GJBaseGameLayer::getItemValue(a, b);
    return x;
}

void GTriggersGJBGL::postInit() {
    this->registerListener();
    this->registerSchedules();

    this->updateCustomItem(ITEM_TOTAL_PLAYERS, 1);
    this->updateCustomItem(ITEM_LEVEL_ID, m_level->m_levelID);
    this->updateCustomItem(ITEM_IS_EDITOR, m_isEditor);
}

void GTriggersGJBGL::registerListener() {
    m_fields->m_listener = NetworkManagerImpl::get().listen<msg::LevelDataMessage>([this](const auto& msg) {
        auto& fields = *m_fields.self();

        for (auto& event : msg.events) {
            this->handleEvent(event);
        }

        bool first = fields.m_firstPacket;
        fields.m_firstPacket = false;

        if (first) {
            this->updateCustomItem(ITEM_LAST_JOINED, cachedSingleton<GJAccountManager>()->m_accountID);
            this->updateCustomItem(ITEM_INITIALIZED, 1);
        }

        for (auto& player : msg.players) {
            // check if the player paused this frame
            bool prevPause = fields.m_pausedPlayers[player.accountId];
            fields.m_pausedPlayers[player.accountId] = player.isPaused;

            if (!prevPause && player.isPaused) {
                this->recordPlayerPause(player.accountId);
            }
        }

        return ListenerResult::Continue;
    });
}

void GTriggersGJBGL::registerSchedules() {
    this->schedule(schedule_selector(GTriggersGJBGL::updateItems));
}

void GTriggersGJBGL::handlePlayerJoin(int accountId) {
    auto& fields = *m_fields.self();

    this->updateCustomItem(ITEM_LAST_JOINED, accountId);
    this->updateCustomItem(ITEM_TOTAL_PLAYERS_JOINED, ++fields.m_totalJoins);
    this->updateCustomItem(ITEM_TOTAL_PLAYERS, GlobedGJBGL::get(this)->m_fields->m_players.size() + 1); // +1 to account for ourself
}

void GTriggersGJBGL::handlePlayerLeave(int accountId) {
    auto& fields = *m_fields.self();

    this->updateCustomItem(ITEM_LAST_LEFT, accountId);
    this->updateCustomItem(ITEM_TOTAL_PLAYERS_LEFT, ++fields.m_totalLeaves);
    this->updateCustomItem(ITEM_TOTAL_PLAYERS, GlobedGJBGL::get(this)->m_fields->m_players.size()); // no +1, because that player is about to be removed

    fields.m_pausedPlayers.erase(accountId);
}

void GTriggersGJBGL::recordPlayerPause(int accountId) {
    auto& fields = *m_fields.self();
    auto now = SystemTime::now();

    this->updateCustomItem(ITEM_LAST_PAUSE_PLAYER, accountId);
    this->updateCustomItem(ITEM_LAST_PAUSE_TIMESTAMP, now.timeSinceEpoch().seconds());
    this->updateCustomItem(ITEM_LAST_PAUSE_TIMESTAMP_MS, now.timeSinceEpoch().subsecMillis());
}

void GTriggersGJBGL::updateItems(float dt) {
    // January 1st 2024
    static SystemTime GLOBED_EPOCH = SystemTime::fromUnix(1704067200);

    auto now = SystemTime::now();
    auto unixTime = now.timeSinceEpoch();
    auto globedTime = now.durationSince(GLOBED_EPOCH).value_or(Duration{});

    this->updateCustomItem(ITEM_UNIX_TIME, unixTime.seconds());
    this->updateCustomItem(ITEM_UNIX_TIME_SUBMS, unixTime.subsecMillis());

    this->updateCustomItem(ITEM_GLEPOCH_TIME, globedTime.seconds());
    this->updateCustomItem(ITEM_GLEPOCH_TIME_SUBMS, globedTime.subsecMillis());

    // assign ping variables
    auto ping = NetworkManagerImpl::get().getGamePing();
    this->updateCustomItem(ITEM_LOCAL_PING, ping.millis());
}

void GTriggersGJBGL::handleEvent(const Event& event) {
    if (event.type != EVENT_COUNTER_CHANGE) {
        return;
    }

    if (event.data.size() < 8) {
        log::warn("Received invalid counter change event with size {}", event.data.size());
        return;
    }

    uint64_t rawData;
    std::memcpy(&rawData, event.data.data(), sizeof(rawData));

    uint8_t rawType = (rawData >> 56) & 0xff;
    uint32_t itemId = (rawData >> 32) & 0x00fffff;
    int32_t value = static_cast<int32_t>(rawData & 0xffffffffULL);

    CounterChange cc{};

    if (rawType != (uint8_t) CounterChangeType::Set) {
        log::warn("Received counter change with unexpected type {}", (int) rawType);
        return;
    }

    cc.type = CounterChangeType::Set;
    cc.itemId = itemId;
    cc.value.asInt() = value;

    log::debug("Got set event for item ID {}, value {}", cc.itemId, cc.value.asInt());

    this->applyCounterChange(cc);
}

void GTriggersGJBGL::applyCounterChange(const CounterChange& change) {
    if (change.type != CounterChangeType::Set) {
        log::warn("applyCounterChange cannot handle type {}", (int) change.type);
        return;
    }

    this->updateCustomItem(change.itemId, change.value.asInt());
}

void GTriggersGJBGL::updateCustomItem(int itemId, int value) {
    if (!globed::isCustomItem(itemId)) {
        log::warn("updateCustomItem called for non-custom item ID {}", itemId);
        return;
    }

    auto efm = static_cast<HookedGJEffectManager*>(m_effectManager);
    if (!efm) {
        return;
    }

    if (efm->updateCountForCustomItem(itemId, value)) {
        this->updateCounters(itemId, value);
    }
}

}
