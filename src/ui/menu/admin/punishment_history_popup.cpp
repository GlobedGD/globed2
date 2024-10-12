#include "punishment_history_popup.hpp"

#include <data/packets/client/admin.hpp>
#include <data/packets/server/admin.hpp>
#include <net/manager.hpp>
#include <ui/general/list/list.hpp>
#include <util/format.hpp>
#include <util/ui.hpp>

#include <asp/time/SystemTime.hpp>

using namespace geode::prelude;
using namespace asp::time;

class AdminPunishmentHistoryPopup::Cell : public CCNode {
public:
    UserPunishment punishment;

    static Cell* create(const UserPunishment& entry, const std::map<int, std::string>& usernames, float width) {
        auto ret = new Cell;
        if (ret->init(entry, usernames, width)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }

    bool init(const UserPunishment& entry, const std::map<int, std::string>& usernames, float width) {
        constexpr float height = 64.f;

        this->punishment = entry;
        this->setContentHeight(height);

        // TODO: icons
        auto icon = Build<CCSprite>::createSpriteName(entry.type == PunishmentType::Ban ? "icon-ban.png"_spr : "icon-mute.png"_spr)
            .with([&] (auto spr) {
                constexpr float pad = 4.f;
                util::ui::rescaleToMatch(spr, {height - pad, height - pad});
            })
            .parent(this)
            .pos(16.f, height / 2.f)
            .collect();

        std::string username;

        if (entry.issuedBy) {
            if (usernames.contains(entry.issuedBy.value())) {
                username = usernames.at(entry.issuedBy.value());
            } else {
                username = GameLevelManager::get()->tryGetUsername(entry.issuedBy.value());
            }

            if (username.empty()) {
                username = "?";
            }
        } else {
            username = "Unknown";
        }

        float startX = icon->getPositionX() + icon->getScaledContentWidth() / 2.f + 5.f;

        auto usernameLabel = Build<CCLabelBMFont>::create(username.c_str(), "goldFont.fnt")
            .limitLabelWidth(width - 50.f, 0.6f, 0.1f)
            .pos(startX, height / 2.f + 20.f)
            .anchorPoint(0.f, 0.5f)
            .parent(this)
            .collect()
            ;

        auto reasonLabel = Build<CCLabelBMFont>::create(entry.reason.empty() ? "No reason given" : entry.reason.c_str(), "goldFont.fnt")
            .limitLabelWidth(width - 50.f, 0.5f, 0.1f)
            .pos(startX, height / 2.f + 8.f)
            .anchorPoint(0.f, 0.5f)
            .parent(this)
            .collect();

        auto issuedAt = SystemTime::fromUnix(entry.issuedAt.value_or(0));
        auto expiresAt = SystemTime::fromUnix(entry.expiresAt);

        std::string dateString;
        if (entry.expiresAt == 0) {
            dateString = fmt::format("{} - Permanent", util::format::formatDateTime(issuedAt));
        } else {
            dateString = fmt::format("{} - {}", util::format::formatDateTime(issuedAt), util::format::formatDateTime(expiresAt));
        }

        Build<CCLabelBMFont>::create(dateString.c_str(), "bigFont.fnt")
            .limitLabelWidth(width - 50.f, 0.35f, 0.1f)
            .pos(reasonLabel->getPositionX(), height / 2.f - 8.f)
            .anchorPoint(0.f, 0.5f)
            .parent(this);

        return true;
    }
};

bool AdminPunishmentHistoryPopup::setup(int32_t accountId) {
    this->setTitle("Punishment history");

    auto rlayout = util::ui::getPopupLayoutAnchored(m_size);

    Build<BetterLoadingCircle>::create()
        .pos(rlayout.center)
        .parent(m_mainLayer)
        .store(loadingCircle);

    loadingCircle->fadeIn();

    auto& nm = NetworkManager::get();
    nm.send(AdminGetPunishmentHistoryPacket::create(accountId));

    nm.addListener<AdminPunishmentHistoryPacket>(this, [this](std::shared_ptr<AdminPunishmentHistoryPacket> packet) {
        loadingCircle->fadeOut();

        if (packet->entries.empty()) {
            auto rlayout = util::ui::getPopupLayoutAnchored(m_size);

            Build<CCLabelBMFont>::create("No punishments", "goldFont.fnt")
                .zOrder(2)
                .pos(rlayout.center)
                .scale(0.5f)
                .parent(m_mainLayer);

            return;
        }

        this->addPunishments(packet->entries, packet->modNameData);
    });

    return true;
}

void AdminPunishmentHistoryPopup::addPunishments(const std::vector<UserPunishment>& entries, const std::map<int, std::string>& usernames) {
    auto rlayout = util::ui::getPopupLayoutAnchored(m_size);

    float width = rlayout.popupSize.width * 0.9f;

    auto list = Build(GlobedListLayer<Cell>::createForComments(width, rlayout.popupSize.height * 0.7f))
        .pos(rlayout.fromCenter(0.f, -10.f))
        .parent(m_mainLayer)
        .collect();

    for (const auto& entry : entries) {
        list->addCell(entry, usernames, width);
    }

    list->sort([](auto a, auto b) {
        return a->punishment.issuedAt > b->punishment.issuedAt;
    });

    list->scrollToTop();
}

AdminPunishmentHistoryPopup* AdminPunishmentHistoryPopup::create(int32_t accountId) {
    auto ret = new AdminPunishmentHistoryPopup;
    if (ret->initAnchored(340.f, 240.f, accountId)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}
