#include "RoomSettingsPopup.hpp"
#include <globed/core/RoomManager.hpp>
#include <globed/core/PopupManager.hpp>
#include <globed/util/CStr.hpp>
#include <core/net/NetworkManagerImpl.hpp>

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

const CCSize RoomSettingsPopup::POPUP_SIZE { 280.f, 210.f};
const CCSize RoomSettingsPopup::LIST_SIZE { POPUP_SIZE.width * 0.9f, POPUP_SIZE.height * 0.75f};
static constexpr float CELL_HEIGHT = 28.f;

namespace {
class Cell : public CCMenu {
public:
    using Callback = std::function<void(bool)>;

    static Cell* create(CStr name, CStr desc, bool value, Callback&& cb) {
        auto ret = new Cell;
        if (ret->init(name, desc, value, std::move(cb))) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }

protected:
    Callback m_callback;

    bool init(CStr name, CStr desc, bool value, Callback&& cb) {
        if (!CCMenu::init()) return false;

        m_callback = std::move(cb);

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
            .intoMenuItem([this, name, desc] {
                globed::alert(name, std::string(desc));
            })
            .pos(ipos)
            .parent(this);

        auto toggler = Build(CCMenuItemExt::createTogglerWithStandardSprites(0.7f, [this](auto toggler) {
            m_callback(!toggler->isOn());
        }))
            .scale(0.85f)
            .pos(this->getContentWidth() - 16.f, CELL_HEIGHT / 2.f)
            .parent(this)
            .collect();

        toggler->toggle(value);

        return true;
    }
};
}

/// Name and desc must be static strings
static Cell* makeCell(CStr name, CStr desc, bool RoomSettings::* bptr) {
    // yeah i hate this too
    bool value = RoomManager::get().getSettings().*bptr;

    return Cell::create(name, desc, value, [bptr](bool state) {
        auto& rm = RoomManager::get();
        auto settings = rm.getSettings();

        settings.*bptr = state;

        NetworkManagerImpl::get().sendUpdateRoomSettings(settings);
    });
}

bool RoomSettingsPopup::setup() {
    this->setTitle("Room Settings");

    m_list = Build(cue::ListNode::create(LIST_SIZE, cue::Brown, cue::ListBorderStyle::Comments))
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
        "Collision",
        "While enabled, players can collide with each other.\n\n<cy>Note: this enables safe mode, making it impossible to make progress on levels.</c>",
        &RoomSettings::collision
    ));

    m_list->addCell(makeCell(
        "2-Player Mode",
        "While enabled, players can link with another player to play a 2-player enabled level together.\n\n<cy>Note: this enables safe mode, making it impossible to make progress on levels.</c>",
        &RoomSettings::twoPlayerMode
    ));

    m_list->addCell(makeCell(
        "Death Link",
        "Whenever a player dies, everyone on the level dies as well. <cy>Inspired by the mod DeathLink from</c> <cg>Alphalaneous</c>.",
        &RoomSettings::deathlink
    ));

    return true;
}

}