#include "RoomSettingsPopup.hpp"
#include <globed/core/PopupManager.hpp>
#include <core/net/NetworkManagerImpl.hpp>

#include <ui/Core.hpp>
#include <asp/iter.hpp>
#include <Geode/utils/function.hpp>

using namespace geode::prelude;

namespace globed {

namespace { namespace $unity {

static constexpr bool USE_LIST = false;
static constexpr float CELL_HEIGHT = 28.f;
static constexpr auto POPUP_SIZE = USE_LIST ? CCSize{ 300.f, 240.f } : CCSize { 310.f, 180.f };
static constexpr CCSize LIST_SIZE { POPUP_SIZE.width * 0.9f, POPUP_SIZE.height * 0.75f};

class Cell : public CCMenu {
public:
    using Callback = geode::Function<void(bool)>;

    static Cell* create(CStr name, CStr desc, bool RoomSettings::* ptr, bool invert, RoomSettingsPopup* popup) {
        auto ret = new Cell;
        if (ret->init(name, desc, ptr, invert, popup)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }

    bool getValue() const {
        return m_toggler->isOn();
    }

protected:
    friend class ::globed::RoomSettingsPopup;
    RoomSettingsPopup* m_popup;
    CStr m_name;
    CCMenuItemToggler* m_toggler;
    bool RoomSettings::* m_ptr;
    bool m_invert;

    bool init(CStr name, CStr desc, bool RoomSettings::* ptr, bool invert, RoomSettingsPopup* popup) {
        if (!CCMenu::init()) return false;

        m_popup = popup;
        m_name = name;
        m_ptr = ptr;
        m_invert = invert;

        this->setContentSize({LIST_SIZE.width, CELL_HEIGHT});

        auto label = Build<CCLabelBMFont>::create(name, "bigFont.fnt")
            .scale(0.5f)
            .anchorPoint(0.f, 0.5f)
            .pos(8.f, CELL_HEIGHT / 2.f + 1.f)
            .parent(this)
            .collect();

        auto ipos = label->getPosition() + CCPoint{label->getScaledContentWidth() + 8.f, 4.5f};

        Build<CCSprite>::createSpriteName("GJ_infoIcon_001.png")
            .scale(0.45f)
            .intoMenuItem([name, desc] {
                globed::alert(name, std::string(desc));
            })
            .pos(ipos)
            .parent(this);

        m_toggler = Build(CCMenuItemExt::createTogglerWithStandardSprites(0.7f, [this](auto toggler) {
            m_popup->onToggled(m_ptr, !toggler->isOn());
        }))
            .scale(0.85f)
            .pos(this->getContentWidth() - 16.f, CELL_HEIGHT / 2.f)
            .parent(this)
            .collect();

        return true;
    }
};

} } // namespace $unity

void RoomSettingsPopup::showSafeModePopup(bool firstTime) {
    auto getSafeModeString = [&]() -> std::string {
        std::vector<std::string> mods;

        if (m_settings.twoPlayerMode) {
            mods.push_back("2-Player Mode");
        }
        if (m_settings.collision) {
            mods.push_back("Collision");
        }
        if (m_settings.switcheroo) {
            mods.push_back("Switcheroo");
        }

        auto joined = utils::string::join(mods, ", ");

        return fmt::format(
            "<cy>Safe mode</c> is <cr>enabled</c> due to the following setting{}: <cy>{}</c>.\n\n"
            "You won't be able to make progress on levels while these settings are enabled.",
            mods.size() > 1 ? "s" : "",
            joined
        );
    };

    if (!m_settings.needsSafeMode()) {
        globed::alert("Safe Mode", "<cy>Safe Mode</c> is <cg>not enabled</c> with these room settings. You are able to make progress on levels while in this room.");
        return;
    }

    globed::alert(
        "Safe Mode",
        firstTime
            ? "This setting enables <cy>safe mode</c>, which means you won't be able to make progress on levels while in this room."
            : getSafeModeString()
    );
}

bool RoomSettingsPopup::init(RoomSettings s) {
    if (!BasePopup::init($unity::POPUP_SIZE)) return false;
    this->setTitle("Room Settings");

    // safe mode btn
    Build<CCSprite>::create("white-period.png"_spr)
        .color(ccGREEN)
        .intoMenuItem([this] {
            this->showSafeModePopup(false);
        })
        .store(m_safeModeBtn)
        .parent(m_buttonMenu)
        .pos(this->fromTopRight(16.f, 16.f));

    m_settings = std::move(s);

    m_cellSetups.emplace_back(
        "Hidden Room",
        "While enabled, the room can not be found on the public room listing and can only be joined by entering the room ID.",
        &RoomSettings::hidden,
        RoomSettingKind::Room
    );

    m_cellSetups.emplace_back(
        "Closed Invites",
        "While enabled, only the room owner can invite other players to the room.",
        &RoomSettings::privateInvites,
        RoomSettingKind::Room
    );

    m_cellSetups.emplace_back(
        "Teams",
        "Enables ability to create and join <cj>Teams</c>. In deathlink, <cy>deaths will only impact your teammates</c>. Otherwise, it only makes a visual difference.",
        &RoomSettings::teams,
        RoomSettingKind::Room
    );

    m_cellSetups.emplace_back(RoomSetting {
        "Auto-pinning",
        "When enabled, any levels the <cg>room owner</c> plays will be automatically pinned and easy to join for other players.",
        &RoomSettings::manualPinning,
        RoomSettingKind::Room,
        {},
        true
    });

    m_cellSetups.emplace_back(RoomSetting {
        "Follower Room",
        "Whenever the <cg>room owner</c> joins any level, other players will be prompted to <cy>follow</c> them to the same level.",
        &RoomSettings::isFollower,
        RoomSettingKind::Room,
    });

    m_cellSetups.emplace_back(RoomSetting {
        "Collision",
        "While enabled, players can collide with each other.\n\n<cy>Note: this enables safe mode, making it impossible to make progress on levels.</c>",
        &RoomSettings::collision,
        RoomSettingKind::Gamemode,
        {&RoomSettings::twoPlayerMode, &RoomSettings::switcheroo}
    });

    m_cellSetups.emplace_back(RoomSetting {
        "2-Player Mode",
        "While enabled, players can link with another player to play a 2-player enabled level together.\n\n<cy>Note: this enables safe mode, making it impossible to make progress on levels.</c>",
        &RoomSettings::twoPlayerMode,
        RoomSettingKind::Gamemode,
        {&RoomSettings::collision, &RoomSettings::deathlink, &RoomSettings::switcheroo}
    });

    m_cellSetups.emplace_back(RoomSetting {
        "Death Link",
        "Whenever a player dies, everyone on the level dies as well. <cy>Inspired by the mod DeathLink from</c> <cg>Alphalaneous</c>.",
        &RoomSettings::deathlink,
        RoomSettingKind::Gamemode,
        {&RoomSettings::twoPlayerMode, &RoomSettings::switcheroo}
    });

    m_cellSetups.emplace_back(RoomSetting {
        "Switcheroo",
        "Players take turns playing the level one at a time, in a random order and at random intervals. Mode activates once the <cj>room owner</c> presses the appropriate button in the pause menu\n\n<cy>Note: this enables safe mode, making it impossible to make progress on levels.</c>",
        &RoomSettings::switcheroo,
        RoomSettingKind::Gamemode,
        {&RoomSettings::collision, &RoomSettings::twoPlayerMode, &RoomSettings::deathlink}
    });

    if constexpr ($unity::USE_LIST) {
        m_list = Build(cue::ListNode::create($unity::LIST_SIZE))
            .pos(this->fromCenter(0.f, -11.f))
            .parent(m_mainLayer);

        for (auto& setup : m_cellSetups) {
            auto cell = $unity::Cell::create(setup.m_name, setup.m_desc, setup.m_ptr, setup.m_invert, this);
            setup.m_toggler = cell->m_toggler;
            m_list->addCell(cell);
        }
    } else {
        float pad = 78.f;
        float padY = -10.f;

        auto leftVert = Build<ColumnContainer>::create(5.f)
            .pos(this->fromLeft({pad, padY}))
            .parent(m_mainLayer)
            .collect();
        auto rightVert = Build<ColumnContainer>::create(5.f)
            .pos(this->fromRight({pad, padY}))
            .parent(m_mainLayer)
            .collect();

        Build<Label>::create("Room Options", "goldFont.fnt")
            .scale(0.55f)
            .parent(leftVert);
        Build<Label>::create("Gameplay Mods", "goldFont.fnt")
            .scale(0.55f)
            .parent(rightVert);

        auto leftContainer = Build<ColumnContainer>::create()
            .parent(leftVert)
            .collect();
        auto rightContainer = Build<ColumnContainer>::create()
            .parent(rightVert)
            .collect();

        leftContainer->layout()->setCrossAxisAlignment(CrossAxisAlignment::Start);
        rightContainer->layout()->setCrossAxisAlignment(CrossAxisAlignment::Start);

        for (auto& setup : m_cellSetups) {
            auto cell = Build<RowContainer>::create()
                .collect();

            Build(CCMenuItemExt::createTogglerWithStandardSprites(0.6f, [this, setup](auto toggler) {
                this->onToggled(setup.m_ptr, !toggler->isOn());
            }))
                .parent(cell)
                .store(setup.m_toggler);

            Build<Label>::create(setup.m_name, "bigFont.fnt")
                .scale(0.37f)
                .anchorPoint(0.f, 0.5f)
                .pos(8.f, 0.f)
                .parent(cell);

            auto infoBtn = Build<CCSprite>::createSpriteName("GJ_infoIcon_001.png")
                .scale(0.35f)
                .intoMenuItem([name = setup.m_name, desc = setup.m_desc] {
                    globed::alert(name, std::string(desc));
                })
                .parent(cell)
                .collect();

            cell->updateLayout();
            infoBtn->setPosition(infoBtn->getPosition() + CCPoint{-2.f, 4.f});

            switch (setup.m_kind) {
                case RoomSettingKind::Room: {
                    leftContainer->addChild(cell);
                } break;
                case RoomSettingKind::Gamemode: {
                    rightContainer->addChild(cell);
                } break;
            }
        }

        leftContainer->updateLayout();
        rightContainer->updateLayout();
        leftVert->updateLayout();
        rightVert->updateLayout();

        cue::attachBackground(leftContainer);
        cue::attachBackground(rightContainer);
    }

    this->reloadCheckboxes();

    return true;
}

void RoomSettingsPopup::onToggled(bool RoomSettings::* bptr, bool state) {
    auto it = std::ranges::find(m_cellSetups, bptr, &RoomSetting::m_ptr);
    if (it == m_cellSetups.end()) return;

    auto& setup = *it;

    if (setup.m_invert) state = !state;
    m_settings.*bptr = state;

    for (auto& incomp : setup.m_incompats) {
        m_settings.*incomp = false;
    }

    FunctionQueue::get().queue([this] {
        this->reloadCheckboxes();
    });

    if (m_callback) {
        m_callback(m_settings);
    }
}

void RoomSettingsPopup::reloadCheckboxes() {
    for (auto& setup : m_cellSetups) {
        if (setup.m_toggler) {
            bool value = m_settings.*setup.m_ptr;
            if (setup.m_invert) value = !value;
            setup.m_toggler->toggle(value);
        }
    }

    bool isSafeMode = m_settings.needsSafeMode();
    if (isSafeMode && !globed::swapFlag("core.flags.seen-room-safe-mode-notice")) {
        this->showSafeModePopup(true);
    }

    m_safeModeBtn->getChildByType<CCSprite>(0)->setColor(isSafeMode ? ccORANGE : ccGREEN);
}

RoomSettingsPopup* RoomSettingsPopup::create(RoomSettings s) {
    auto ret = new RoomSettingsPopup;
    if (ret->init(std::move(s))) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

}