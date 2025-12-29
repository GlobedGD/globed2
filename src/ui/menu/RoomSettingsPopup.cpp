#include "RoomSettingsPopup.hpp"
#include <globed/core/PopupManager.hpp>
#include <core/net/NetworkManagerImpl.hpp>

#include <UIBuilder.hpp>
#include <std23/move_only_function.h>

using namespace geode::prelude;

namespace globed {

const CCSize RoomSettingsPopup::POPUP_SIZE { 280.f, 210.f};
const CCSize RoomSettingsPopup::LIST_SIZE { POPUP_SIZE.width * 0.9f, POPUP_SIZE.height * 0.75f};
static constexpr float CELL_HEIGHT = 28.f;

namespace {
class Cell : public CCMenu {
public:
    using Callback = std23::move_only_function<void(bool)>;

    static Cell* create(CStr name, CStr desc, bool RoomSettings::* ptr, bool invert, Callback&& cb) {
        auto ret = new Cell;
        if (ret->init(name, desc, ptr, invert, std::move(cb))) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }

    void reload() {
        auto& settings = RoomManager::get().getSettings();
        bool value = settings.*m_ptr;
        if (m_invert) value = !value;
        m_toggler->toggle(value);
    }

    bool getValue() const {
        return m_toggler->isOn();
    }

protected:
    Callback m_callback;
    CStr m_name;
    CCMenuItemToggler* m_toggler;
    bool RoomSettings::* m_ptr;
    bool m_invert;

    bool init(CStr name, CStr desc, bool RoomSettings::* ptr, bool invert, Callback&& cb) {
        if (!CCMenu::init()) return false;

        m_callback = std::move(cb);
        m_name = name;
        m_ptr = ptr;
        m_invert = invert;

        this->setContentSize({RoomSettingsPopup::LIST_SIZE.width, CELL_HEIGHT});

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
            m_callback(!toggler->isOn());
        }))
            .scale(0.85f)
            .pos(this->getContentWidth() - 16.f, CELL_HEIGHT / 2.f)
            .parent(this)
            .collect();

        this->reload();

        return true;
    }
};
}

/// Name and desc must be static strings
CCNode* RoomSettingsPopup::makeCell(
    CStr name,
    CStr desc,
    bool RoomSettings::* bptr,
    std::vector<bool RoomSettings::*> incompats,
    bool invert
) {
    return Cell::create(name, desc, bptr, invert, [this, bptr, invert, incompats = std::move(incompats)](bool state) {
        auto& rm = RoomManager::get();
        auto& settings = rm.getSettings();

        if (invert) state = !state;
        settings.*bptr = state;

        for (auto& incomp : incompats) {
            settings.*incomp = false;
        }

        FunctionQueue::get().queue([this] {
            this->reloadCheckboxes();
        });

        NetworkManagerImpl::get().sendUpdateRoomSettings(settings);
    });
}

bool RoomSettingsPopup::setup() {
    this->setTitle("Room Settings");

    m_list = Build(cue::ListNode::create(LIST_SIZE))
        .pos(this->fromCenter(0.f, -11.f))
        .parent(m_mainLayer);

    m_list->addCell(makeCell(
        "Hidden Room",
        "While enabled, the room can not be found on the public room listing and can only be joined by entering the room ID.",
        &RoomSettings::hidden
    ));

    m_list->addCell(makeCell(
        "Closed Invites",
        "While enabled, only the room owner can invite other players to the room.",
        &RoomSettings::privateInvites
    ));

    m_list->addCell(makeCell(
        "Teams",
        "Enables ability to create and join <cj>Teams</c>. In deathlink, <cy>deaths will only impact your teammates</c>. Otherwise, it only makes a visual difference.",
        &RoomSettings::teams
    ));

    m_list->addCell(makeCell(
        "Auto-pinning",
        "When enabled, any levels the <cg>room owner</c> plays will be automatically pinned and easy to join for other players.",
        &RoomSettings::manualPinning,
        {},
        true
    ));

    m_list->addCell(makeCell(
        "Collision",
        "While enabled, players can collide with each other.\n\n<cy>Note: this enables safe mode, making it impossible to make progress on levels.</c>",
        &RoomSettings::collision,
        {&RoomSettings::twoPlayerMode, &RoomSettings::switcheroo}
    ));

    m_list->addCell(makeCell(
        "2-Player Mode",
        "While enabled, players can link with another player to play a 2-player enabled level together.\n\n<cy>Note: this enables safe mode, making it impossible to make progress on levels.</c>",
        &RoomSettings::twoPlayerMode,
        {&RoomSettings::collision, &RoomSettings::deathlink, &RoomSettings::switcheroo}
    ));

    m_list->addCell(makeCell(
        "Death Link",
        "Whenever a player dies, everyone on the level dies as well. <cy>Inspired by the mod DeathLink from</c> <cg>Alphalaneous</c>.",
        &RoomSettings::deathlink,
        {&RoomSettings::twoPlayerMode, &RoomSettings::switcheroo}
    ));

    m_list->addCell(makeCell(
        "Switcheroo",
        "Players take turns playing the level one at a time, in a random order and at random intervals. Mode activates once the <cj>room owner</c> presses the appropriate button in the pause menu\n\n<cy>Note: this enables safe mode, making it impossible to make progress on levels.</c>",
        &RoomSettings::switcheroo,
        {&RoomSettings::collision, &RoomSettings::twoPlayerMode, &RoomSettings::deathlink}
    ));

    return true;
}

void RoomSettingsPopup::reloadCheckboxes() {
    for (auto cell : m_list->iter<Cell>()) {
        cell->reload();
    }
}

}