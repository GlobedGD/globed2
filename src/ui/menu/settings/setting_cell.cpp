#include "setting_cell.hpp"

#include <asp/math/NumberCycle.hpp>

#include "audio_setup_popup.hpp"
#include "keybind_settings_popup.hpp"
#include "string_input_popup.hpp"
#include "advanced_settings_popup.hpp"
#include "link_code_popup.hpp"
#include "swag_connection_test_popup.hpp"
#include "connection_test_popup.hpp"
#include <managers/settings.hpp>
#include <managers/central_server.hpp>
#include <managers/popup.hpp>
#include <net/manager.hpp>
#include <ui/general/ask_input_popup.hpp>
#include <ui/general/slider.hpp>
#include <ui/menu/settings/keybind_setup_popup.hpp>
#include <util/format.hpp>
#include <util/misc.hpp>
#include <util/rng.hpp>

#ifdef GEODE_IS_WINDOWS
# include <XInput.h>
#endif

using namespace geode::prelude;
namespace permission = geode::utils::permission;
using permission::Permission;

bool GlobedSettingCell::init(void* settingStorage, Type settingType, const char* nameText, const char* descText, const Limits& limits) {
    if (!CCLayer::init()) return false;
    this->settingStorage = settingStorage;
    this->settingType = settingType;
    this->descText = descText;
    this->limits = limits;

    Build<CCLabelBMFont>::create(nameText, "bigFont.fnt")
        .scale(0.5f)
        .anchorPoint(0.f, 0.5f)
        .pos(10.f, CELL_HEIGHT / 2)
        .parent(this)
        .store(labelName);

    // desc button
    if (descText && descText[0] != '\0') {
        auto labelSize = labelName->getScaledContentSize();
        Build<CCSprite>::createSpriteName("GJ_infoIcon_001.png")
            .scale(0.35f)
            .intoMenuItem([this, nameText, descText](auto) {
                std::string text = descText;

                if (this->anyController && !GlobedSettings::get().flags.seenSwagConnectionPopup) {
                    text += "\n\n<cg>LB DOWN ??? RB</c>";
                }

                PopupManager::get().alert(nameText, text).showInstant();
            })
            .pos(10.f + labelSize.width + 5.f, CELL_HEIGHT / 2 + labelSize.height / 2 - 5.f)
            .store(btnInfo)
            .intoNewParent(CCMenu::create())
            .pos(0.f, 0.f)
            .parent(this);
    }

    switch (settingType) {
        case Type::Bool: {
            Build<CCMenuItemToggler>(CCMenuItemToggler::createWithStandardSprites(this, menu_selector(GlobedSettingCell::onCheckboxToggled), 0.85f))
                .anchorPoint(0.5f, 0.5f)
                .pos(CELL_WIDTH - 15.f, CELL_HEIGHT / 2)
                .scale(0.8f)
                .id("input-checkbox"_spr)
                .store(inpCheckbox)
                .intoNewParent(CCMenu::create())
                .pos(0.f, 0.f)
                .id("input-menu"_spr)
                .parent(this);

            inpCheckbox->toggle(*(bool*)(settingStorage));
        } break;
        case Type::DiscordRPC: {
            bool possible = Loader::get()->isModLoaded("techstudent10.discord_rich_presence");
            float opacity = possible ? 1.f : 0.5f;

            Build<CCMenuItemToggler>(CCMenuItemToggler::createWithStandardSprites(this, menu_selector(GlobedSettingCell::onCheckboxToggled), 0.85f))
                .anchorPoint(0.5f, 0.5f)
                .pos(CELL_WIDTH - 15.f, CELL_HEIGHT / 2)
                .scale(0.8f)
                .with([&](auto* btn) {
                    btn->m_offButton->setOpacity(static_cast<unsigned char>(255 * opacity));
                    btn->m_onButton->setOpacity(static_cast<unsigned char>(255 * opacity));
                })
                .id("input-checkbox"_spr)
                .store(inpCheckbox)
                .intoNewParent(CCMenu::create())
                .pos(0.f, 0.f)
                .id("input-menu"_spr)
                .parent(this)
                .enabled(possible);

            if (!possible) {
                Build<CCSprite>::createSpriteName("GJ_infoIcon_001.png")
                    .scale(0.35f)
                    .intoMenuItem([](auto) {
                        PopupManager::get().alert("Not available", "This feature requires the Discord Rich Presence mod to be installed.").showInstant();
                    })
                    .pos(inpCheckbox->getPositionX() - 20.f, inpCheckbox->getPositionY() + 10.f)
                    .intoNewParent(CCMenu::create())
                    .pos(0.f, 0.f)
                    .parent(this);

                this->storeAndSave(false);
            }

            inpCheckbox->toggle(*(bool*)(settingStorage));
        } break;
        case Type::Float: {
            Build<BetterSlider>::create()
                .anchorPoint(1.f, 0.5f)
                .pos(CELL_WIDTH - 12.f, CELL_HEIGHT / 2 - 1.f)
                .parent(this)
                .id("input-slider"_spr)
                .store(inpSlider);

            inpSlider->setCallback([this](auto* slider, double value) {
                this->onSliderChanged(slider, value);
            });
            inpSlider->setLimits(limits.floatMin, limits.floatMax);
            inpSlider->setContentWidth(90.f);

            float value = *(float*)(settingStorage);
            inpSlider->setValue(value);
        } break;
        case Type::String: [[fallthrough]];
        case Type::PacketFragmentation: [[fallthrough]];
        case Type::AdvancedSettings: [[fallthrough]];
        case Type::LinkCode: [[fallthrough]];
        case Type::ConnectionTest: [[fallthrough]];
        case Type::KeybindSettings: [[fallthrough]];
        case Type::AudioDevice: {
            const char* text;
            switch (settingType) {
                case Type::PacketFragmentation: text = "Auto"; break;
                case Type::AdvancedSettings: text = "View"; break;
                case Type::AudioDevice: text = "Set"; break;
                case Type::LinkCode: text = "View"; break;
                case Type::ConnectionTest: text = "Test"; break;
                case Type::KeybindSettings: text = "View"; break;
                default: text = "what the figma"; break;
            }

            Build<ButtonSprite>::create(text, "goldFont.fnt", "GJ_button_04.png", .7f)
                .scale(0.8f)
                .intoMenuItem([this](auto* sender) {
                    this->onInteractiveButton(sender);
                })
                .scaleMult(1.1f)
                .anchorPoint(0.5f, 0.5f)
                .pos(CELL_WIDTH - 8.f, CELL_HEIGHT / 2)
                .id("input-interactive-btn"_spr)
                .store(inpAudioButton)
                .intoNewParent(CCMenu::create())
                .pos(0.f, 0.f)
                .id("input-menu"_spr)
                .parent(this);

            inpAudioButton->setPositionX(inpAudioButton->getPositionX() - inpAudioButton->getScaledContentSize().width / 2);

            if (settingType == Type::ConnectionTest) {
                this->checkForController();
            }
        } break;
        case Type::Int: {
            int currentValue = *(int*)(settingStorage);
            Build<TextInput>::create(CELL_WIDTH * 0.2f, "", "chatFont.fnt")
                .scale(0.8f)
                .anchorPoint(1.f, 0.5f)
                .pos(CELL_WIDTH - 8.f, CELL_HEIGHT / 2)
                .id("input-field"_spr)
                .store(inpField)
                .intoNewParent(CCMenu::create())
                .pos(0.f, 0.f)
                .id("input-menu"_spr)
                .parent(this);

            inpField->setCommonFilter(CommonFilter::Uint);
            inpField->setMaxCharCount(10);
            inpField->setCallback([this](const std::string& string) {
                auto val = util::format::parse<int>(string);
                if (val) {
                    int vval = val.value();
                    if (this->limits.intMax != 0 && this->limits.intMin != 0) {
                        vval = std::clamp(vval, this->limits.intMin, this->limits.intMax);
                    }

                    this->storeAndSave(val.value());
                }
            });

            inpField->setString(std::to_string(currentValue));
        } break;
        case Type::Corner: {
            this->recreateCornerButton();
        } break;
        case Type::InvitesFrom: {
            this->recreateInvitesFromButton();
        } break;
    }

    if (auto* menu = this->getChildByID("input-menu"_spr)) {
        menu->setLayout(RowLayout::create()
                            ->setAxisAlignment(AxisAlignment::End)
                            ->setAxisReverse(true)
                            ->setAutoScale(false));
        menu->setContentWidth(CELL_WIDTH - 5.f);
        menu->setAnchorPoint({0.f, 0.5f});
        menu->setPositionX(-1.f);
        menu->setPositionY(CELL_HEIGHT / 2.f);

        if (settingType == Type::PacketFragmentation) {
            auto spr = Build<CCSprite>::createSpriteName("pencil.png"_spr)
                .collect();

            // button to manually edit packet frag
            Build<CircleButtonSprite>::create(spr)
                .scale(0.5f)
                .intoMenuItem([this, settingStorage] (auto) {
                    AskInputPopup::create("Packet limit", [this](auto input) {
                        auto limit = util::format::parse<uint32_t>(input).value_or(0);
                        if ((limit > 0 && limit < 1300) || limit > 65535) {
                            PopupManager::get().alert("Error", "<cr>Invalid</c> limit was set. For best results, use the <cg>Connection Test</c> option, and only use this option if you know what you're doing.").showInstant();
                            return;
                        }

                        this->storeAndSave((int) limit);
                    }, 6, std::to_string(*(int*)settingStorage), util::misc::STRING_DIGITS, 10.f)->show();
                })
                .id("fraglimit-manual-btn"_spr)
                .parent(menu);
        }

        menu->updateLayout();
    }

    return true;
}

void GlobedSettingCell::onCheckboxToggled(cocos2d::CCObject*) {
    this->storeAndSave(!inpCheckbox->isOn());
}

void GlobedSettingCell::onSliderChanged(BetterSlider* slider, double value) {
    this->storeAndSave(static_cast<float>(value));
}

void GlobedSettingCell::onInteractiveButton(cocos2d::CCObject*) {
    switch (settingType) {
        case Type::AudioDevice: this->onSetAudioDevice(); break;
        case Type::PacketFragmentation: {
            PopupManager::get().alert("Error", "Packet limit test is now <cr>obsolete</c>, please use the <cg>Connection Test</c> setting, which will perform the same test at the end.").showInstant();
            // if (NetworkManager::get().established()) {
            //     FragmentationCalibartionPopup::create()->show();
            // } else {
            //     PopupManager::get().alert("Error", "This action can only be done when connected to a server.").showInstant();
            // }
            break;
        }
        case Type::AdvancedSettings: {
            AdvancedSettingsPopup::create()->show();
            break;
        }
        case Type::LinkCode: {
            if (NetworkManager::get().established()) {
                LinkCodePopup::create()->show();
            } else {
                PopupManager::get().alert("Error", "This action can only be done when connected to a server.").showInstant();
            }
            break;
        }

        case Type::ConnectionTest: {
            ConnectionTestPopup::create()->show();
        } break;

        case Type::KeybindSettings: {
            KeybindSettingsPopup::create()->show();
        } break;

        default: {
            StringInputPopup::create([this](std::string_view text) {
                this->onStringChanged(text);
            })->show();
            break;
        }
    }
}

void GlobedSettingCell::onSetAudioDevice() {
#ifdef GLOBED_VOICE_SUPPORT
# ifndef GLOBED_VOICE_CAN_TALK
    // if we can't talk, show an error popup
    PopupManager::get().alert("Error", "Sorry, but recording audio is currently <cr>not possible</c> on this platform.").showInstant();
    return;
# endif // GLOBED_VOICE_CAN_TALK

    // check for permission
    bool perm = permission::getPermissionStatus(Permission::RecordAudio);

    if (!perm) {
        geode::createQuickPopup(
            "No permission",
            "Globed does not currently have permission to use your microphone. Do you want to grant the permission?",
            "Cancel", "Grant", [] (FLAlertLayer*, bool btn2) {

            if (btn2) {
                permission::requestPermission(Permission::RecordAudio, [](bool granted) {
                    if (granted) {
                        AudioSetupPopup::create()->show();
                    } else {
                        log::warn("permission denied when requesting audio access");
                    }
                });
            }
        });

        return;
    }
    AudioSetupPopup::create()->show();
#endif // GLOBED_VOICE_SUPPORT
}

void GlobedSettingCell::onStringChanged(std::string_view text) {
    this->storeAndSave(text);
}

void GlobedSettingCell::recreateCornerButton() {
    if (cornerButton) {
        cornerButton->getParent()->removeFromParent();
        cornerButton->removeFromParent();
        cornerButton = nullptr;
    }

    int currentValue = *(int*)(settingStorage);

    ButtonSprite* theButton;

    Build<ButtonSprite>::create("000", "bigFont.fnt", "GJ_button_04.png", 0.4f)
        .scale(0.6f)
        .store(theButton)
        .intoMenuItem([this, currentValue](auto) {
            int cv = currentValue + 1;
            if (cv > 3) {
                cv = 0;
            }

            this->storeAndSave(cv);
            this->recreateCornerButton();
        })
        .anchorPoint(0.5f, 0.5f)
        .pos(CELL_WIDTH - 23.f, CELL_HEIGHT / 2)
        .scaleMult(1.1f)
        .id("overlay-btn")
        .store(cornerButton)
        .intoNewParent(CCMenu::create())
        .pos(0.f, 0.f)
        .id("overlay-menu")
        .parent(this);

    // kill me
    float xOff = 9.f * ((currentValue % 2 == 1) ? 1.f : -1.f);
    float yOff = 8.f * ((currentValue < 2) ? 1.f : -1.f);
    theButton->m_label->setPosition(theButton->m_label->getPosition() + ccp(xOff + 1.f, yOff));
    theButton->m_label->setString("0");
}

void GlobedSettingCell::recreateInvitesFromButton() {
    using InvitesFrom = GlobedSettings::InvitesFrom;

    if (invitesFromButton) {
        invitesFromButton->getParent()->removeFromParent();
        invitesFromButton->removeFromParent();
        invitesFromButton = nullptr;
    }

    InvitesFrom currentValue = static_cast<InvitesFrom>(std::clamp(*(int*)(settingStorage), 0, 2));

    const char* text = "";
    switch (currentValue) {
        case InvitesFrom::Everyone: text = "Everyone"; break;
        case InvitesFrom::Friends: text = "Friends"; break;
        case InvitesFrom::Nobody: text = "Nobody"; break;
        default: globed::unreachable();
    }

    Build<ButtonSprite>::create(text, "bigFont.fnt", "GJ_button_04.png", 0.5f)
        .scale(0.6f)
        .intoMenuItem([this, currentValue](auto) {
            asp::NumberCycle curValue((int)currentValue, 0, (int)InvitesFrom::Nobody);
            curValue.increment();

            this->storeAndSave(curValue.get());
            this->recreateInvitesFromButton();
        })
        .anchorPoint(0.5f, 0.5f)
        .with([](auto* btn) {
            btn->setPosition(CELL_WIDTH - 6.f - btn->getScaledContentSize().width / 2.f, CELL_HEIGHT / 2);
        })
        .scaleMult(1.1f)
        .id("invite-from-btn")
        .store(invitesFromButton)
        .intoNewParent(CCMenu::create())
        .pos(0.f, 0.f)
        .id("invite-from-menu")
        .parent(this);
}

void GlobedSettingCell::storeAndSave(std::any&& value) {
    // banger
    switch (settingType) {
        case Type::DiscordRPC: [[fallthrough]];
        case Type::Bool:
            *(bool*)(settingStorage) = std::any_cast<bool>(value); break;
        case Type::Float:
            *(float*)(settingStorage) = std::any_cast<float>(value); break;
        case Type::String:
            *(std::string*)(settingStorage) = std::any_cast<std::string>(std::move(value)); break;
        case Type::AudioDevice: [[fallthrough]];
        case Type::Corner: [[fallthrough]];
        case Type::PacketFragmentation: [[fallthrough]];
        case Type::InvitesFrom: [[fallthrough]];
        case Type::LinkCode: [[fallthrough]];
        case Type::Int:
            *(int*)(settingStorage) = std::any_cast<int>(value); break;
        case Type::ConnectionTest: [[fallthrough]];
        case Type::KeybindSettings: [[fallthrough]];
        case Type::AdvancedSettings:
            break;
    }

    GlobedSettings::get().save();
}

#ifdef GEODE_IS_WINDOWS
DWORD XInputGetState_call(DWORD dwUserIndex, XINPUT_STATE* state) {
    static auto module = [] {
        auto x14 = GetModuleHandleA("XInput1_4.dll");
        if (x14) {
            return x14;
        }

        return GetModuleHandleA("XInput9_1_0.dll");
    }();

    static auto exportfn = GetProcAddress(module, "XInputGetState");
    if (!exportfn) {
        log::error("Failed to load XInput!");
        return -1;
    }

    return reinterpret_cast<decltype(&XInputGetState)>(exportfn)(dwUserIndex, state);
}
#endif

void GlobedSettingCell::checkForController() {
#ifdef GEODE_IS_WINDOWS
    for (DWORD i = 0; i < XUSER_MAX_COUNT; i++) {
        XINPUT_STATE state{};

        DWORD res = XInputGetState_call(i, &state);
        pluggedControllers[i] = res == ERROR_SUCCESS;

        if (pluggedControllers[i]) {
            this->anyController = true;
        }
    }

    if (this->anyController) {
        this->schedule(schedule_selector(GlobedSettingCell::updateController));

        controllerSeq = util::misc::ButtonSequence{{
            // https://learn.microsoft.com/en-us/windows/win32/api/xinput/ns-xinput-xinput_gamepad
            0x0100, 0x0002, 0x8000, 0x0200
        }};
        controllerSeq.setBitComps(true);

        if (!GlobedSettings::get().flags.seenSwagConnectionPopup) {
            this->btnInfo->setColor({0, 255, 180});
        }
    }
#endif
}

void GlobedSettingCell::updateController(float) {
#ifdef GEODE_IS_WINDOWS
    for (size_t i = 0; i < XUSER_MAX_COUNT; i++) {
        if (!this->pluggedControllers[i]) continue;

        XINPUT_STATE state{};
        if (XInputGetState_call(i, &state) != ERROR_SUCCESS) {
            pluggedControllers[i] = false;
            continue;
        }

        if (controllerLastPktNum[i] == state.dwPacketNumber) {
            continue;
        }

        controllerLastPktNum[i] = state.dwPacketNumber;
        if (state.Gamepad.wButtons == 0) {
            continue;
        }

        // check for button presses
        bool completed = controllerSeq.pushButton(state.Gamepad.wButtons);
        if (completed) {
            // first check if any popups are present already!
            if (CCScene::get()->getChildByType<FLAlertLayer>(0)) {
                return;
            }

            GlobedSettings::get().flags.seenSwagConnectionPopup = true;
            SwagConnectionTestPopup::create()->animateIn();
        }
    }
#endif
}

GlobedSettingCell* GlobedSettingCell::create(void* settingStorage, Type settingType, const char* name, const char* desc, const Limits& limits) {
    auto ret = new GlobedSettingCell;
    if (ret->init(settingStorage, settingType, name, desc, limits)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}