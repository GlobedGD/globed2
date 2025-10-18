#include "SetupFireServerPopup.hpp"
#include <globed/core/PopupManager.hpp>
#include <globed/core/data/Event.hpp>

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

const CCSize SetupFireServerPopup::POPUP_SIZE {400.f, 270.f};

bool SetupFireServerPopup::setup(FireServerObject* obj) {
    m_object = obj;
    m_payload = this->getPayload();

    this->setTitle("Setup Send Event");
    this->setID("setup-send-event-popup"_spr);

    m_title->setPositionY(m_title->getPositionY() + 5.f);
    m_closeBtn->setVisible(false);

    auto rootNode = Build<CCNode>::create()
        .anchorPoint(0.f, 0.5f)
        .pos(this->fromLeft({2.f, 8.f}))
        .layout(ColumnLayout::create()->setGap(-2.f)->setAxisReverse(true))
        .contentSize(POPUP_SIZE.width * 0.8f, POPUP_SIZE.height * 0.75f)
        .parent(m_mainLayer)
        .id("root-node")
        .collect();

    for (size_t i = 0; i < 5; i++) {
        // note that we on purpose set even values that are not covered by argcount
        int value = m_payload.args[i].value;

        auto node = this->createParam(i, value);
        node->setPosition(0, i * 50);
        rootNode->addChild(node);
    }

    rootNode->updateLayout();

    Build(this->createInputBox(5, m_payload.eventId))
        .pos(this->fromTopRight(66.f, 80.f))
        .parent(m_mainLayer);

    Build<CCLabelBMFont>::create("Event ID", "chatFont.fnt")
        .scale(0.8f)
        .pos(this->fromTopRight(66.f, 50.f))
        .parent(m_mainLayer);

    // add ok button
    Build(ButtonSprite::create("OK", "goldFont.fnt", "GJ_button_01.png", 1.0f))
        .scale(0.75f)
        .intoMenuItem([this] {
            this->onClose(nullptr);
        })
        .pos(this->fromBottom(21.f))
        .parent(m_buttonMenu);

    // add spawn triggered / touch triggered / multi trigger checkboxes

    auto spawnTrigger = this->addBaseCheckbox("Spawn\nTrigger", &m_spawnTriggerBtn, [this](auto toggler) {
        bool on = !toggler->isOn();

        this->setTriggerState(on, false);
    });

    auto touchTrigger = this->addBaseCheckbox("Touch\nTrigger", &m_touchTriggerBtn, [this](auto toggler) {
        bool on = !toggler->isOn();

        this->setTriggerState(false, on);
    });

    m_multiTriggerNode = this->addBaseCheckbox("Multi\nTrigger", &m_multiTriggerBtn, [this](auto toggler) {
        bool on = !toggler->isOn();
        m_object->m_isMultiTriggered = on;
    });

    m_spawnTriggerBtn->toggle(m_object->m_isSpawnTriggered);
    m_touchTriggerBtn->toggle(m_object->m_isTouchTriggered);
    m_multiTriggerBtn->toggle(m_object->m_isMultiTriggered);

    Build<CCNode>::create()
        .layout(RowLayout::create()->setGap(5.f))
        .anchorPoint(0.5f, 0.5f)
        .contentSize(POPUP_SIZE.width * 0.4f, 24.f)
        .child(spawnTrigger)
        .child(touchTrigger)
        .pos(this->fromBottomLeft(88.f, 22.f))
        .parent(m_mainLayer)
        .updateLayout();

    m_multiTriggerNode->setPosition(this->fromBottomRight(45.f, 22.f));
    m_mainLayer->addChild(m_multiTriggerNode);

    return true;
}

CCNode* SetupFireServerPopup::createParam(size_t idx, int value) {
    auto radioMenu = Build<CCNode>::create()
        .layout(RowLayout::create()->setGap(5.5f))
        .contentSize(136.f, 64.f)
        .collect();

    auto makeToggler = [&](const char* name, FireServerArgType type) {
        auto toggler = CCMenuItemExt::createTogglerWithStandardSprites(0.8f, [this, idx, type, radioMenu](auto toggler) {
            bool on = !toggler->isOn();

            if (on) {
                for (auto item : radioMenu->getChildrenExt<CCNode>()) {
                    auto target = item->getChildByType<CCMenuItemToggler>(0);

                    if (target != toggler) {
                        log::debug("buh: {}", item);
                        target->toggle(false);
                    }
                }

                this->onArgTypeChange(idx, type);
            } else {
                this->onArgTypeChange(idx, FireServerArgType::None);
            }
        });

        if (m_payload.args[idx].type == type) {
            toggler->toggle(true);
        }

        Build<CCMenu>::create()
            .layout(ColumnLayout::create()->setAutoScale(false))
            .contentSize(36.f, 48.f)
            .child(toggler)
            .child(Build<CCLabelBMFont>::create(name, "bigFont.fnt").limitLabelWidth(36.f, 0.45f, 0.1f))
            .parent(radioMenu)
            .updateLayout();
    };

    makeToggler("Item", FireServerArgType::Item);
    makeToggler("Timer", FireServerArgType::Timer);
    makeToggler("Static", FireServerArgType::Static);
    radioMenu->updateLayout();

    return Build(this->createInputBox(idx, value))
        .child(
            Build<CCLabelBMFont>::create(fmt::format("Param {}", idx + 1).c_str(), "chatFont.fnt")
                .scale(0.6f)
        )
        .child(radioMenu)
        .updateLayout();
}

CCNode* SetupFireServerPopup::createInputBox(size_t idx, int value) {
    auto input = TextInput::create(60.f, "0");
    input->setCommonFilter(CommonFilter::Int);

    if (value == 0) {
        input->setString("");
    } else {
        input->setString(fmt::to_string(value));
    }

    input->setCallback([this, input, idx](const std::string& str) {
        if (str.empty()) {
            this->onPropChange(idx, 0);
            return;
        }

        auto res = geode::utils::numFromString<int>(str);
        if (!res) {
            input->setString("");
            this->onPropChange(idx, 0);
            return;
        }

        this->onPropChange(idx, res.unwrap());
    });

    auto menu = Build<CCMenu>::create()
        .layout(RowLayout::create()->setGap(7.5f)->setAutoScale(false))
        .contentSize(POPUP_SIZE.width * 0.8f, 50.f)
        .child(
            Build<CCSprite>::createSpriteName("edit_leftBtn_001.png")
                .intoMenuItem([this, idx, input](auto) {
                    auto res = geode::utils::numFromString<int>(input->getString());

                    if (!res) {
                        input->setString("0");
                        this->onPropChange(idx, 0);
                        return;
                    }

                    input->setString(fmt::to_string(res.unwrap() - 1));
                    this->onPropChange(idx, res.unwrap() - 1);
                })
        )
        .child(input)
        .child(
            Build<CCSprite>::createSpriteName("edit_rightBtn_001.png")
                .intoMenuItem([this, idx, input](auto) {
                    auto res = geode::utils::numFromString<int>(input->getString());

                    if (!res) {
                        input->setString("0");
                        this->onPropChange(idx, 0);
                        return;
                    }

                    input->setString(fmt::to_string(res.unwrap() + 1));
                    this->onPropChange(idx, res.unwrap() + 1);
                })
        )
        .updateLayout();

    return menu;
}

CCNode* SetupFireServerPopup::addBaseCheckbox(const char* label, CCMenuItemToggler** store, std23::move_only_function<void(CCMenuItemToggler*)> callback) {
    auto toggler = CCMenuItemExt::createTogglerWithStandardSprites(0.8f, std::move(callback));

    *store = toggler;

    return Build<CCMenu>::create()
        .layout(RowLayout::create()->setAutoScale(false)->setGap(6.f))
        .contentSize(80.f, 24.f)
        .child(toggler)
        .child(Build<CCLabelBMFont>::create(label, "bigFont.fnt").scale(0.35f))
        .updateLayout();
}

void SetupFireServerPopup::setTriggerState(bool spawn, bool touch) {
    m_multiTriggerNode->setVisible(spawn || touch);

    m_object->m_isSpawnTriggered = false;
    m_object->m_isTouchTriggered = false;

    if (spawn) {
        m_object->m_isSpawnTriggered = true;
        m_touchTriggerBtn->toggle(false);
    } else if (touch) {
        m_object->m_isTouchTriggered = true;
        m_spawnTriggerBtn->toggle(false);
    }
}

void SetupFireServerPopup::onClose(CCObject*) {
    // detect skips
    for (size_t i = m_payload.argCount; i < 5; i++) {
        if (m_payload.args[i].type != FireServerArgType::None) {
            globed::alert("Error", "Please set the event parameters in order, without skipping any.");
            return;
        }
    }

    // detect invalid event ID
    if (m_invalidEventId) {
        globed::alertFormat("Error", "Please set a valid event ID. It must range from 0 to {}", EVENT_GLOBED_BASE - 1);
        return;
    }

    BasePopup::onClose(nullptr);

    // commit all changes
    m_object->encodePayload(m_payload);
}

void SetupFireServerPopup::onPropChange(size_t idx, int value) {
    if (idx == 5) {
        // event id
        if (value < 0 || value >= EVENT_GLOBED_BASE) {
            log::warn("Invalid event ID {} for FireServerObject", value);
            m_invalidEventId = true;
            return;
        }

        m_invalidEventId = false;
        m_payload.eventId = value;
    } else if (idx < 5) {
        // arg value
        m_payload.args[idx].value = value;
    } else {
        log::warn("Invalid property index {} for FireServerObject", idx);
        return;
    }
}

void SetupFireServerPopup::onArgTypeChange(size_t idx, FireServerArgType type) {
    if (idx < 5) {
        m_payload.args[idx].type = type;
    } else {
        log::warn("Invalid argument index {} for FireServerObject", idx);
        return;
    }

    // set the arg count to the appropriate value, note there cannot be any skips
    m_payload.argCount = 0;
    for (size_t i = 0; i < 5; i++) {
        if (m_payload.args[i].type != FireServerArgType::None) {
            m_payload.argCount++;
        } else {
            break;
        }
    }
}

FireServerPayload SetupFireServerPopup::getPayload() {
    auto payloadRes = m_object->decodePayload();
    if (!payloadRes) {
        return {};
    }

    return *payloadRes;
}

}
