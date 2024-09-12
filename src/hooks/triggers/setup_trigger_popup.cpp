#include <defs/platform.hpp>
#include <globed/constants.hpp>
#include <util/format.hpp>

#include "setup_pickup_trigger_popup.hpp"
#include "setup_instant_count_popup.hpp"

using namespace geode::prelude;

constexpr int ItemId = 0x50;
constexpr int ItemId2 = 95;
constexpr int ItemId3 = 51;

#include <Geode/modify/SetupTriggerPopup.hpp>
struct GLOBED_DLL SetupPopupHook : geode::Modify<SetupPopupHook, SetupTriggerPopup> {
    $override
    void valueChanged(int p0, float p1) {
        bool removeLimits = \
            (p0 == ItemId || p0 == ItemId2 || p0 == ItemId3) \
            && (util::misc::is_any_of_dynamic<SetupItemCompareTriggerPopup, SetupItemEditTriggerPopup>(this));

        std::optional<bool> putBack;
        if (removeLimits) {
            if (m_shouldLimitValues.contains(p0)) {
                putBack = m_shouldLimitValues.at(p0);
                m_shouldLimitValues.at(p0) = false;
            }
        }

        // ItemId3 has extra checks here
        if (p0 == ItemId3) {
            int val = std::max(0, (int)p1);

            if (m_gameObject) {
                m_gameObject->m_targetGroupID = val;
            } else if (m_gameObjects) {
                for (auto obj : CCArrayExt<EffectGameObject*>(m_gameObjects)) {
                    obj->m_targetGroupID = val;
                }
            }
            this->valueDidChange(p0, val);
        } else {
            SetupTriggerPopup::valueChanged(p0, p1);
        }

        if (putBack.has_value()) {
            m_shouldLimitValues.at(p0) = putBack.value();
        }

        if (auto p = typeinfo_cast<SetupPickupTriggerPopup*>(this)) {
            static_cast<PickupPopupHook*>(p)->onUpdateValue(p0, p1);
        }
    }

    $override
    void textChanged(CCTextInputNode* inputNode) {
        if (inputNode->getTag() != ItemId || m_disableTextDelegate) {
            return SetupTriggerPopup::textChanged(inputNode);
        }

        auto p = static_cast<PickupPopupHook*>(typeinfo_cast<SetupPickupTriggerPopup*>(this));
        if (!p) {
            return SetupTriggerPopup::textChanged(inputNode);
        }

        bool wasGlobedMode = p->m_fields->globedMode;
        SetupTriggerPopup::textChanged(inputNode);

        if (wasGlobedMode) {
            int id = util::format::parse<int>(inputNode->getString()).value_or(0);
            p->onUpdateValue(ItemId, globed::CUSTOM_ITEM_ID_W_START + id);
        }
    }
};
