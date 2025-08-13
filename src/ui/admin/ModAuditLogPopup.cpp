#include "ModAuditLogPopup.hpp"
#include <core/net/NetworkManagerImpl.hpp>

#include <UIBuilder.hpp>
#include <cue/PlayerIcon.hpp>

using namespace geode::prelude;

namespace globed {

const CCSize ModAuditLogPopup::POPUP_SIZE = {360.f, 240.f};
static constexpr float TYPE_HEIGHT = 20.f;
static constexpr float TYPE_WIDTH = 155.f;
static constexpr float USER_HEIGHT = 20.f;
static constexpr float USER_WIDTH = 155.f;

struct ActionType {
    std::string_view id;
    const char* iconSprite;
    const char* name;
};

class ActionTypeCell : public CCNode {
public:
    static ActionTypeCell* create(const ActionType& type) {
        auto ret = new ActionTypeCell;
        ret->init(type);
        ret->autorelease();
        return ret;
    }

private:
    bool init(const ActionType& type) {
        if (!CCNode::init()) return false;

        this->setContentSize({TYPE_WIDTH, TYPE_HEIGHT});

        Build<CCSprite>::create(type.iconSprite)
            .with([&](auto spr) { cue::rescaleToMatch(spr, TYPE_HEIGHT * 0.8f); })
            .pos(12.f, TYPE_HEIGHT / 2.f)
            .parent(this);

        Build<CCLabelBMFont>::create(type.name, "bigFont.fnt")
            .limitLabelWidth(TYPE_WIDTH * 0.7f, 0.45f, 0.1f)
            .anchorPoint(0.f, 0.5f)
            .pos(24.f, TYPE_HEIGHT / 2.f)
            .parent(this);

        return true;
    }
};

class ModCell : public CCNode {
public:
    static ModCell* create(const FetchedMod& mod) {
        auto ret = new ModCell;
        ret->init(mod);
        ret->autorelease();
        return ret;
    }

    static ModCell* createNone() {
        auto ret = new ModCell;
        ret->init(FetchedMod { .accountId = 0 });
        ret->autorelease();
        return ret;
    }

    int m_accountId;

private:
    bool init(const FetchedMod& mod) {
        if (!CCNode::init()) return false;

        m_accountId = mod.accountId;

        this->setContentSize({USER_WIDTH, USER_HEIGHT});

        if (mod.accountId == 0) {
            Build<CCLabelBMFont>::create("Everyone", "bigFont.fnt")
                .limitLabelWidth(TYPE_WIDTH * 0.7f, 0.45f, 0.1f)
                .anchorPoint(0.f, 0.5f)
                .pos(4.f, TYPE_HEIGHT / 2.f)
                .parent(this);
        } else {
            // boring ui
            Build(cue::PlayerIcon::create(
                IconType::Cube,
                mod.cube,
                mod.color1,
                mod.color2,
                mod.glowColor == NO_GLOW ? -1 : (int)mod.glowColor
            ))
                .with([&](auto spr) { cue::rescaleToMatch(spr, USER_HEIGHT * 0.8f); })
                .pos(12.f, USER_HEIGHT / 2.f)
                .parent(this);

            Build<CCLabelBMFont>::create(mod.username.c_str(), "bigFont.fnt")
                .limitLabelWidth(TYPE_WIDTH * 0.7f, 0.45f, 0.1f)
                .anchorPoint(0.f, 0.5f)
                .pos(24.f, TYPE_HEIGHT / 2.f)
                .parent(this);
        }

        return true;
    }
};

bool ModAuditLogPopup::setup() {
    ccColor4B bg{ 105, 61, 31, 255 };

    m_typeDropdown = Build(cue::DropdownNode::create(bg, TYPE_WIDTH, TYPE_HEIGHT, 130.f))
        .anchorPoint(0.5f, 1.f)
        .pos(this->fromTopLeft(90.f, 20.f))
        .parent(m_mainLayer);

    constexpr static auto types = std::to_array<ActionType>({
        ActionType{ "kick", "button-admin-kick.png"_spr, "Kick" },
        ActionType{ "notice", "button-admin-notice.png"_spr, "Notice" },
        ActionType{ "ban", "button-admin-ban.png"_spr, "Ban" },
        ActionType{ "editban", "button-admin-ban.png"_spr, "Edit ban" },
        ActionType{ "unban", "button-admin-unban.png"_spr, "Unban" },
        ActionType{ "mute", "button-admin-mute.png"_spr, "Mute" },
        ActionType{ "editmute", "button-admin-mute.png"_spr, "Edit mute" },
        ActionType{ "unmute", "button-admin-unmute.png"_spr, "Unmute" },
        ActionType{ "roomban", "button-admin-room-ban.png"_spr, "Room ban" },
        ActionType{ "editroomban", "button-admin-room-ban.png"_spr, "Edit room ban" },
        ActionType{ "roomunban", "button-admin-room-unban.png"_spr, "Room unban" },
        ActionType{ "editroles", "role-mod.png"_spr, "Edit roles" },
        ActionType{ "editpassword", "button-admin-password.png"_spr, "Edit password" },
    });

    for (auto& type : types) {
        m_typeDropdown->addCell(ActionTypeCell::create(type));
    }

    m_typeDropdown->setCallback([this](size_t idx, CCNode* cell) {
        m_chosenType = types[idx].id;
        this->refetch();
    });

    // user dropdown

    m_userDropdown = Build(cue::DropdownNode::create(bg, USER_WIDTH, USER_HEIGHT, 130.f))
        .anchorPoint(0.5f, 1.f)
        .pos(this->fromTopRight(90.f, 20.f))
        .parent(m_mainLayer);

    m_userDropdown->addCell(ModCell::createNone());

    m_userDropdown->setCallback([this](size_t idx, CCNode* cell) {
        m_chosenUserId = static_cast<ModCell*>(cell)->m_accountId;
        this->refetch();
    });

    // fetch users
    auto& nm = NetworkManagerImpl::get();
    m_modsListener = nm.listen<msg::AdminFetchModsResponseMessage>([this](const auto& msg) {
        this->populateMods(msg.users);
        return ListenerResult::Continue;
    });

    nm.sendAdminFetchMods();

    return true;
}

void ModAuditLogPopup::populateMods(const std::vector<FetchedMod>& mods) {
    for (auto& mod : mods) {
        m_userDropdown->addCell(ModCell::create(mod));
    }
}

void ModAuditLogPopup::refetch() {
    auto& nm = NetworkManagerImpl::get();
    // TODO
}

}