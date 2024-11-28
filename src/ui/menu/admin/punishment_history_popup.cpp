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
        constexpr float height = 48.f;

        this->punishment = entry;
        this->setContentHeight(height);

        // TODO: icons
        auto icon = Build<CCSprite>::createSpriteName(entry.type == PunishmentType::Ban ? "button-admin-ban.png"_spr : "button-admin-mute.png"_spr)
            .with([&] (auto spr) {
                constexpr float pad = 4.f;
                util::ui::rescaleToMatch(spr, {32.f, 32.f});
            })
            .parent(this)
            .pos(22.f, height / 2.f)
            .collect();

        std::string username;

        if (entry.issuedBy) {
            if (usernames.contains(entry.issuedBy.value())) {
                username = usernames.at(entry.issuedBy.value());
            } else {
                username = GameLevelManager::get()->tryGetUsername(entry.issuedBy.value());
            }

            if (username.empty()) {
                username = fmt::format("Unknown ({})", entry.issuedBy.value());
            }
        } else {
            username = "Unknown";
        }

        float startX = icon->getPositionX() + icon->getScaledContentWidth() / 2.f + 5.f;

        auto usernameLabel = Build<CCLabelBMFont>::create(username.c_str(), "goldFont.fnt")
            .limitLabelWidth(width - 50.f, 0.6f, 0.1f)
            .pos(startX, height / 2.f + 14.f)
            .anchorPoint(0.f, 0.5f)
            .parent(this)
            .collect()
            ;

        auto reasonLabel = Build<CCLabelBMFont>::create(entry.reason.empty() ? "No reason given" : entry.reason.c_str(), "bigFont.fnt")
            .color(entry.reason.empty() ? ccColor3B{175, 175, 175} : ccColor3B{255, 255, 255})
            .limitLabelWidth(width - 50.f, 0.4f, 0.1f)
            .pos(startX, height / 2.f + 0.f)
            .anchorPoint(0.f, 0.5f)
            .parent(this)
            .collect();

        SystemTime issuedAt = SystemTime::fromUnix(entry.issuedAt.value_or(0));;
        SystemTime expiresAt = SystemTime::fromUnix(entry.expiresAt);

        bool isPermanent = expiresAt.to_time_t() == 0;

        std::string issuedStr, expiresStr;

        // sanity check
        if (expiresAt < issuedAt && !isPermanent) {
            issuedStr = "Unknown";
            expiresStr = "Unknown";
        }
        // if the issued time is unknown, the expire time is shown as a date, otherwise it's shown as a duration
        else if (issuedAt.to_time_t() == 0) {
            issuedStr = "Unknown";
            expiresStr = isPermanent ? "Permanent" : util::format::formatDateTime(expiresAt, false);
        } else {
            issuedStr = util::format::formatDateTime(issuedAt, false);
            expiresStr = isPermanent ? "Permanent" : expiresAt.durationSince(issuedAt)->toHumanString();
        }

        std::string dateString = fmt::format("{} - {}", issuedStr, expiresStr);

        Build<CCLabelBMFont>::create(dateString.c_str(), "bigFont.fnt")
            .limitLabelWidth(width - 50.f, 0.25f, 0.05f)
            .pos(reasonLabel->getPositionX(), height / 2.f - 14.f)
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
