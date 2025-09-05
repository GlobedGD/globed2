#include "ModAuditLogPopup.hpp"
#include <core/net/NetworkManagerImpl.hpp>

#include <UIBuilder.hpp>
#include <cue/PlayerIcon.hpp>
#include <asp/time/SystemTime.hpp>

using namespace geode::prelude;
using namespace asp::time;

namespace globed {

const CCSize ModAuditLogPopup::POPUP_SIZE = {380.f, 280.f};
static constexpr float TYPE_HEIGHT = 20.f;
static constexpr float TYPE_WIDTH = 155.f;
static constexpr float USER_HEIGHT = 20.f;
static constexpr float USER_WIDTH = 155.f;

static std::string formatDateTime(const asp::time::SystemTime& tp, bool ms) {
    std::time_t curTime = tp.to_time_t();

    if (ms) {
        auto millis = tp.timeSinceEpoch().subsecMillis();
        return fmt::format("{:%Y-%m-%d %H:%M:%S}.{:03}", fmt::localtime(curTime), millis);
    } else {
        return fmt::format("{:%Y-%m-%d %H:%M:%S}", fmt::localtime(curTime));
    }
}

struct ActionType {
    std::string_view id;
    const char* iconSprite;
    const char* name;
};

constexpr static auto ACTION_TYPES = std::to_array<ActionType>({
    ActionType{ "", "icon-person.png"_spr, "Everything" },
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

const ActionType* findActionType(std::string_view id) {
    for (auto& a : ACTION_TYPES) {
        if (a.id == id) {
            return &a;
        }
    }

    return nullptr;
}

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

class LogCell : public CCMenu {
public:
    static inline const float WIDTH = ModAuditLogPopup::POPUP_SIZE.width * 0.9f;
    static inline const float HEIGHT = 48.f;
    static inline const float EXPANDED_HEIGHT = 100.f;

    static LogCell* create(const AdminAuditLog& log, const PlayerAccountData& issuer, const PlayerAccountData& target) {
        auto ret = new LogCell;
        if (ret->init(log, issuer, target)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }

private:
    CCNode* m_collapsedContainer;

    bool init(const AdminAuditLog& log, const PlayerAccountData& issuer, const PlayerAccountData& target) {
        if (!CCMenu::init()) return false;

        this->ignoreAnchorPointForPosition(false);
        this->setContentSize({WIDTH, HEIGHT});

        m_collapsedContainer = Build<CCMenu>::create()
            .ignoreAnchorPointForPos(false)
            .anchorPoint(0.f, 0.f)
            .pos(0.f, 0.f)
            .contentSize(WIDTH, HEIGHT)
            .parent(this);

        auto action = findActionType(log.type);
        if (!action) {
            Build<CCLabelBMFont>::create(fmt::format("Unknown action: {}", log.type).c_str(), "bigFont.fnt")
                .scale(0.5f)
                .anchorPoint(0.f, 0.5f)
                .pos(8.f, HEIGHT / 2.f)
                .parent(m_collapsedContainer);

            return true;
        }

        Build<CCSprite>::create(action->iconSprite)
            .with([&](auto spr) { cue::rescaleToMatch(spr, HEIGHT * 0.6f); })
            .pos(20.f, HEIGHT / 2.f - 2.f)
            .parent(m_collapsedContainer);


        std::string_view targetStr;

        if (log.type == "notice" && log.targetId == 0) {
            targetStr = "Multiple people";
        } else {
            targetStr = target.username;
        }

        Build<CCLabelBMFont>::create(fmt::format("{} by {} for {}", action->name, issuer.username, targetStr).c_str(), "goldFont.fnt")
            .scale(0.45f)
            .anchorPoint(0.f, 0.5f)
            .pos(40.f, HEIGHT * 0.75f + 1.f)
            .parent(m_collapsedContainer);

        std::string msgStr;

        bool isPunishment =
            log.type == "kick"
            || log.type == "mute"
            || log.type == "editmute"
            || log.type == "ban"
            || log.type == "editban"
            || log.type == "roomban"
            || log.type == "editroomban";

        bool isUnpunishment =
            log.type == "unmute"
            || log.type == "unban"
            || log.type == "roomunban";

        if (isPunishment) {
            msgStr = fmt::format("Reason: {}", log.message);
        } else if (log.type == "editroles") {
            // TODO: format more neatly
            msgStr = log.message;
        }

        if (!msgStr.empty()) {
            Build<CCLabelBMFont>::create(msgStr.c_str(), "bigFont.fnt")
                .limitLabelWidth(WIDTH * 0.84f, 0.4f, 0.05f)
                .anchorPoint(0.f, 0.5f)
                .pos(40.f, HEIGHT * 0.5f - 2.f)
                .parent(m_collapsedContainer);
        }

        if (isPunishment) {
            std::string issuedStr, expiresStr;

            SystemTime issuedAt = SystemTime::fromUnix(log.timestamp);
            SystemTime expiresAt = SystemTime::fromUnix(log.expiresAt);
            bool isPermanent = log.expiresAt == 0;

            // sanity check
            if (expiresAt < issuedAt && !isPermanent) {
                issuedStr = "Unknown";
                expiresStr = "Unknown";
            }
            // if the issued time is unknown, the expire time is shown as a date, otherwise it's shown as a duration
            else if (issuedAt.to_time_t() == 0) {
                issuedStr = "Unknown";
                expiresStr = isPermanent ? "Permanent" : formatDateTime(expiresAt, false);
            } else {
                issuedStr = formatDateTime(issuedAt, false);
                expiresStr = isPermanent ? "Permanent" : expiresAt.durationSince(issuedAt)->toHumanString();
            }

            std::string footer;

            Build<CCLabelBMFont>::create(footer.c_str(), "bigFont.fnt")
                .limitLabelWidth(WIDTH * 0.84f, 0.4f, 0.05f)
                .anchorPoint(0.f, 0.5f)
                .pos(40.f, HEIGHT * 0.2f)
                .parent(m_collapsedContainer);
        }

        return true;
    }

    void expand(bool expanded) {
        this->setContentHeight(expanded ? EXPANDED_HEIGHT : HEIGHT);
        m_collapsedContainer->setPositionY(expanded ? EXPANDED_HEIGHT - HEIGHT : 0.f);

        // TODO expansion
    }
};

bool ModAuditLogPopup::setup() {
    ccColor4B bg{ 105, 61, 31, 255 };

    m_loadingCircle = cue::LoadingCircle::create();
    m_loadingCircle->addToLayer(m_mainLayer);
    m_loadingCircle->setZOrder(5);

    m_typeDropdown = Build(cue::DropdownNode::create(bg, TYPE_WIDTH, TYPE_HEIGHT, 130.f))
        .zOrder(11)
        .anchorPoint(0.5f, 1.f)
        .pos(this->fromTopLeft(90.f, 20.f))
        .parent(m_mainLayer);

    for (auto& type : ACTION_TYPES) {
        m_typeDropdown->addCell(ActionTypeCell::create(type));
    }

    m_typeDropdown->setCallback([this](size_t idx, CCNode* cell) {
        m_filters.type = ACTION_TYPES[idx].id;
        this->refetch();
    });

    // user dropdown

    m_userDropdown = Build(cue::DropdownNode::create(bg, USER_WIDTH, USER_HEIGHT, 130.f))
        .zOrder(11)
        .anchorPoint(0.5f, 1.f)
        .pos(this->fromTopRight(90.f, 20.f))
        .parent(m_mainLayer);

    m_userDropdown->addCell(ModCell::createNone());

    m_userDropdown->setCallback([this](size_t idx, CCNode* cell) {
        m_filters.issuer = static_cast<ModCell*>(cell)->m_accountId;
        this->refetch();
    });

    // log list

    m_list = Build(cue::ListNode::create({LogCell::WIDTH, 200.f}, cue::Brown, cue::ListBorderStyle::Comments))
        .pos(this->fromCenter(0.f, -15.f))
        .parent(m_mainLayer);

    // fetch users
    auto& nm = NetworkManagerImpl::get();
    m_modsListener = nm.listen<msg::AdminFetchModsResponseMessage>([this](const auto& msg) {
        this->populateMods(msg.users);
        return ListenerResult::Continue;
    });

    m_logsListener = nm.listen<msg::AdminLogsResponseMessage>([this](const auto& msg) {
        this->populateLogs(msg.logs, msg.users);
        return ListenerResult::Continue;
    });

    nm.sendAdminFetchMods();
    this->refetch();

    return true;
}

void ModAuditLogPopup::populateMods(const std::vector<FetchedMod>& mods) {
    for (auto& mod : mods) {
        m_userDropdown->addCell(ModCell::create(mod));
    }
}

void ModAuditLogPopup::populateLogs(const std::vector<AdminAuditLog>& logs, const std::vector<PlayerAccountData>& users) {
    m_loadReqs--;
    m_loadingCircle->fadeOut();

    m_list->setAutoUpdate(false);
    m_list->clear();

    for (const auto& log : logs) {
        const PlayerAccountData* issuer = nullptr;
        const PlayerAccountData* target = nullptr;

        static PlayerAccountData INVALID_DATA {
            .accountId = 0,
            .userId = 0,
            .username = "Unknown"
        };

        // locate the issuer and target
        for (const auto& user : users) {
            if (user.accountId == log.accountId) {
                issuer = &user;
            } else if (user.accountId == log.targetId) {
                target = &user;
            }

            if (issuer && target) break;
        }

        m_list->addCell(LogCell::create(log, issuer ? *issuer : INVALID_DATA, target ? *target : INVALID_DATA));
    }

    m_list->setAutoUpdate(true);
    m_list->updateLayout();
}

void ModAuditLogPopup::refetch() {
    m_loadReqs++;

    if (m_loadReqs == 1) {
        m_loadingCircle->fadeIn();
    }

    auto& nm = NetworkManagerImpl::get();
    nm.sendAdminFetchLogs(m_filters);
}

}