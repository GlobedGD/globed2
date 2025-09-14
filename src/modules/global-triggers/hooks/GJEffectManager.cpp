#include "GJEffectManager.hpp"
#include <core/hooks/GJBaseGameLayer.hpp>
#include "../Ids.hpp"

using namespace geode::prelude;

static bool g_dontUpdateCountTriggers = false;

namespace globed {

int HookedGJEffectManager::countForItem(int itemId) {
    if (!isCustomItem(itemId)) {
        itemId = std::clamp(itemId, 0, 9999);
        return m_itemCountMap[itemId];
    }

    return this->countForCustomItem(itemId);
}

int HookedGJEffectManager::countForCustomItem(int itemId) {
    return m_fields->m_customItems[itemId];
}

void HookedGJEffectManager::updateCountForItem(int itemId, int value) {
    if (isWritableCustomItem(itemId)) {
        this->updateCountForCustomItem(itemId, value);
    } else {
        GJEffectManager::updateCountForItem(itemId, value);
    }
}

bool HookedGJEffectManager::updateCountForCustomItem(int itemId, int value) {
    auto& fields = *m_fields.self();

    int prev = fields.m_customItems[itemId];
    fields.m_customItems[itemId] = value;

    bool ret = prev != value;

    if (g_dontUpdateCountTriggers) {
        return ret;
    }

    // update count triggers haha
    // please dont even begin to try to understand any of this
    // TODO clean this up

    if (!m_countTriggerActions.contains(itemId)) {
        return ret;
    }

    auto& actions = m_countTriggerActions.at(itemId);

    auto comparator = value < prev
        ? [](const CountTriggerAction& a, const CountTriggerAction& b){ return b.m_targetCount < a.m_targetCount; }
        : [](const CountTriggerAction& a, const CountTriggerAction& b){ return a.m_targetCount < b.m_targetCount; };

    // yeah whatever you say robby
    std::sort(actions.begin(), actions.end(), comparator);

    for (size_t i = 0; i < actions.size(); ) {
        auto& action = actions[i];

        if (action.m_previousCount == value) {
            i++;
            continue;
        }

        bool multiActivate = action.m_multiActivate;
        int prevCount = action.m_previousCount;
        action.m_previousCount = value;

        auto stuff = [&]{
            auto unkVecInt = action.m_remapKeys;
            bool unk10 = action.m_activateGroup;
            int unk14 = action.m_triggerUniqueID;
            int unk18 = action.m_controlID;
            int groupId = action.m_targetGroupID;

            if (!action.m_multiActivate) {
                actions.erase(actions.begin() + i);
            }

            if (!m_triggerEffectDelegate) {
                // i cant be bothered im not gonna lie
                // this->toggleGroup(groupId, unk10);
            } else {
                m_triggerEffectDelegate->toggleGroupTriggered(groupId, unk10, unkVecInt, unk14, unk18);
            }
        };

        if (action.m_targetCount <= prevCount) {
            if ((action.m_targetCount < prevCount) && (value <= action.m_targetCount)) {
                stuff();
            } else {
                i++;
                continue;
            }
        } else if (value < action.m_targetCount) {
            i++;
            continue;
        } else {
            stuff();
        }

        // log::debug("Action: 0 = {}, 4 = {}, 8 = {}, c = {}, 10 = {}, 14 = {}, 18 = {}, 1c = {}, set = {}", action.m_unk0, action.m_unk4, action.m_unk8, action.m_unkc, action.m_unk10, action.m_unk14, action.m_unk18, action.m_unk1c, action.m_unkVecInt);
        // m_triggerEffectDelegate->toggleGroupTriggered(id)

        if (multiActivate) {
            i++;
        }
    }

    return ret;
}

void HookedGJEffectManager::reset() {
    GJEffectManager::reset();

    // dont actually reset
    // m_fields->m_customItems.clear();
}

}