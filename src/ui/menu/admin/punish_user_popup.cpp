#include "punish_user_popup.hpp"

#include <defs/geode.hpp>
#include <data/packets/client/admin.hpp>
#include <net/manager.hpp>
#include <util/format.hpp>
#include <util/ui.hpp>
#include <ui/general/list/list.hpp>

#include "user_popup.hpp"

#include <asp/time/SystemTime.hpp>

using namespace geode::prelude;
using namespace asp::time;

class AdminPunishUserPopup::CommonReasonPopup : public geode::Popup<AdminPunishUserPopup*, bool> {
public:
    static CommonReasonPopup* create(AdminPunishUserPopup* popup, bool isBan) {
        auto ret = new CommonReasonPopup;
        if (ret->initAnchored(400.f, 260.f, popup, isBan)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }

    void onSelected(const char* text) {
        this->popup->setReason(text);
        this->onClose(nullptr);
    }

private:
    AdminPunishUserPopup* popup;

    class Cell : public CCNode {
    public:
        static Cell* create(AdminPunishUserPopup::CommonReasonPopup* popup, const char* text, float cellWidth, float cellHeight) {
            auto ret = new Cell;
            if (ret->init(popup, text, cellWidth, cellHeight)) {
                ret->autorelease();
                return ret;
            }

            delete ret;
            return nullptr;
        }

    protected:
        bool init(AdminPunishUserPopup::CommonReasonPopup* popup, const char* text, float cellWidth, float cellHeight) {
            Build<CCLabelBMFont>::create(text, "bigFont.fnt")
                .limitLabelWidth(cellWidth - 32.f, 0.45f, 0.1f)
                .anchorPoint(0.f, 0.5f)
                .pos(5.f, cellHeight / 2.f)
                .parent(this);

            this->setContentWidth(cellWidth);

            Build<CCSprite>::createSpriteName("btn_chatHistory_001.png")
                .with([&](auto spr) {
                    util::ui::rescaleToMatch(spr, {cellHeight - 4.f, cellHeight - 4.f});
                })
                .intoMenuItem([text, popup] {
                    popup->onSelected(text);
                })
                .with([&](CCMenuItemSpriteExtra* btn) {
                    btn->setPosition({ cellWidth - 5.f - btn->getScaledContentWidth() / 2.f, cellHeight / 2.f });
                })
                .intoNewParent(CCMenu::create())
                .parent(this)
                .pos(0.f, 0.f)
                .contentSize({cellWidth, cellHeight});

            return true;
        }
    };

    bool setup(AdminPunishUserPopup* popup, bool isBan) {
        this->setTitle("Common Reasons");

        this->popup = popup;

        auto rlayout = util::ui::getPopupLayoutAnchored(m_size);

        float cellWidth = rlayout.popupSize.width * 0.9f;
        constexpr float cellHeight = 24.f;

        auto list = Build(GlobedListLayer<Cell>::createForComments(cellWidth, rlayout.popupSize.height * 0.7f, cellHeight))
            .anchorPoint(0.5f, 0.5f)
            .pos(rlayout.center)
            .parent(m_mainLayer)
            .collect();

        auto reasons =
            isBan ? std::initializer_list<const char*>{
                "Inappropriate room name",
                "Hate speech / harassment",
                "Inappropriate username",
                "Advertising through the use of room names",
                "Ban transferred from external platform",
                "Inappropriate usage of voice chat",
                "Ban evasion"
            }

            : // mute reasons

            std::initializer_list<const char*>{
                "Micspamming (music / soundboard)",
                "Micspamming (unreasonably loud)",
                "Enabling microphone without speaking (clicking / background noise)",
                "Toxicity / harassment",
                "Inappropriate behavior"
            };

        for (auto reason : reasons) {
            list->addCell(this, reason, cellWidth, cellHeight);
        }

        if (popup->accountId == 18950870) {
            list->addCell(this, "Being exceptionally gay :3", cellWidth, cellHeight);
        }

        list->scrollToTop();

        return true;
    }
};

bool AdminPunishUserPopup::setup(AdminUserPopup* popup, int32_t accountId, bool isBan, std::optional<UserPunishment> punishment_) {
    this->setTitle(isBan ? "Ban/Unban user" : "Mute/Unmute user");
    this->punishment = std::move(punishment_);
    this->accountId = accountId;
    this->isBan = isBan;
    this->parentPopup = popup;

    auto rlayout = util::ui::getPopupLayoutAnchored(m_size);
    auto* rootLayout = Build<CCNode>::create()
        .contentSize(m_size.width * 0.9f, m_size.height * 0.7f)
        .pos(rlayout.fromCenter(0.f, -20.f))
        .anchorPoint(0.5f, 0.5f)
        .layout(
            ColumnLayout::create()
                ->setAutoScale(false)
                ->setAxisReverse(true)
            )
        .parent(m_mainLayer)
        .collect();

    auto* reasonLayout = Build<CCMenu>::create()
        .contentSize(rootLayout->getScaledContentSize())
        .layout(RowLayout::create()->setAutoScale(false))
        .parent(rootLayout)
        .collect();

    // text input
    Build<TextInput>::create(reasonLayout->getContentWidth() * 0.75f, "Reason", "chatFont.fnt")
        .store(reasonInput)
        .with([&](TextInput* input) {
            input->setCommonFilter(CommonFilter::Any);
        })
        .parent(reasonLayout);

    Build<CCSprite>::createSpriteName("GJ_closeBtn_001.png")
        .scale(0.68f)
        .intoMenuItem([this] {
            this->reasonInput->setString("");
        })
        .parent(reasonLayout);

    Build<CCSprite>::createSpriteName("btn_chatHistory_001.png")
        .intoMenuItem([this, isBan] {
            CommonReasonPopup::create(this, isBan)->show();
        })
        .parent(reasonLayout);

    reasonLayout->updateLayout();

    Build<CCLabelBMFont>::create(isBan ? "Ban Duration" : "Mute Duration", "bigFont.fnt")
        .scale(0.5f)
        .parent(rootLayout)
        ;

    auto durRoot = Build<CCNode>::create()
        .layout(RowLayout::create())
        .contentSize(rootLayout->getScaledContentWidth(), 92.f)
        .parent(rootLayout)
        .collect();

    auto durGrid = Build<CCMenu>::create()
        .layout(RowLayout::create()->setAutoScale(false)->setGap(3.f)->setGrowCrossAxis(true))
        .parent(durRoot)
        .contentSize(m_size.width * 0.5f, durRoot->getScaledContentHeight())
        .collect();

    constexpr static auto HOUR = Duration::fromHours(1);
    constexpr static auto DAY = Duration::fromDays(1);
    constexpr static auto MONTH = Duration::fromDays(30);

    // quick buttons for duration
    for (Duration off : std::initializer_list<Duration>{
        HOUR * 6,
        DAY * 1,
        DAY * 3,
        DAY * 7,
        DAY * 14,
        DAY * 30,
        MONTH * 3,
        MONTH * 6,
        MONTH * 12,
        Duration{}
    }) {
        std::string labeltext;
        float scale = 0.9f;

        if (off.isZero()) {
            labeltext = "Permanent";
            scale = 0.675f;
        } else if (off >= MONTH) {
            auto months = off.days() / MONTH.days();
            labeltext = fmt::format("{}mo", months);
            scale = 0.835f;
        } else if (off >= DAY) {
            auto days = off.days();
            labeltext = fmt::format("{}d", days);
        } else {
            auto hrs = off.hours();
            labeltext = fmt::format("{}h", hrs);
        }

        auto* offSprite = Build<ButtonSprite>::create(labeltext.c_str(), "bigFont.fnt", "GJ_button_04.png", scale)
            .scale(0.55f)
            .collect();

        auto* onSprite = Build<ButtonSprite>::create(labeltext.c_str(), "bigFont.fnt", "GJ_button_02.png", scale)
            .scale(0.55f)
            .collect();

        auto toggler = CCMenuItemExt::createToggler(onSprite, offSprite, [this, off](auto btn) {
            bool clear = btn->isToggled();

            if (clear) {
                btn->toggle(false);
            } else {
                this->setDuration(off, true);
            }
        });

        toggler->m_onButton->m_scaleMultiplier = 1.15f;
        toggler->m_offButton->m_scaleMultiplier = 1.15f;

        toggler->setTag((int) off.seconds());
        durGrid->addChild(toggler);
        durationButtons.try_emplace(off, toggler);
    }

    durGrid->updateLayout();

    // inputs for custom duration
    Build<CCNode>::create()
        .layout(
            ColumnLayout::create()
                ->setAxisReverse(true)
                ->setAutoScale(false)
                ->setGap(3.f)
        )
        .parent(durRoot)
        .contentSize(32.f, durRoot->getScaledContentHeight())

        // Days
        .child(
            Build<CCLabelBMFont>::create("Days", "bigFont.fnt")
                .scale(0.4f)
        )
        .child(
            Build<CCMenu>::create()
                .contentSize(100.f, 0.f)
                .layout(RowLayout::create()->setAutoScale(false))
                .child(
                    Build<CCSprite>::createSpriteName("edit_leftBtn_001.png")
                        .scale(0.9f)
                        .intoMenuItem([this] {
                            auto newdu = currentDuration - Duration::fromDays(1);

                            this->setDuration(newdu);
                        })
                )
                .child(
                    Build<TextInput>::create(64.f, "0", "bigFont.fnt")
                        .with([&](TextInput* inp) {
                            inp->setCommonFilter(CommonFilter::Uint);
                            inp->setCallback([this] (auto) {
                                this->inputChanged();
                            });
                        })
                        .scale(0.65f)
                        .store(daysInput)
                )
                .child(
                    Build<CCSprite>::createSpriteName("edit_rightBtn_001.png")
                        .scale(0.9f)
                        .intoMenuItem([this] {
                            auto newdu = currentDuration + Duration::fromDays(1);
                            this->setDuration(newdu);
                        })
                )
                .updateLayout()
        )

        // Hours
        .child(
            Build<CCLabelBMFont>::create("Hours", "bigFont.fnt")
                .scale(0.4f)
        )
        .child(
            Build<CCMenu>::create()
                .contentSize(100.f, 0.f)
                .layout(RowLayout::create()->setAutoScale(false))
                .child(
                    Build<CCSprite>::createSpriteName("edit_leftBtn_001.png")
                        .scale(0.9f)
                        .intoMenuItem([this] {
                            auto newdu = currentDuration - Duration::fromHours(1);
                            this->setDuration(newdu);
                        })
                )
                .child(
                    Build<TextInput>::create(64.f, "0", "bigFont.fnt")
                        .with([&](TextInput* inp) {
                            inp->setCommonFilter(CommonFilter::Uint);
                            inp->setCallback([this] (auto) {
                                this->inputChanged();
                            });
                        })
                        .scale(0.65f)
                        .store(hoursInput)
                )
                .child(
                    Build<CCSprite>::createSpriteName("edit_rightBtn_001.png")
                        .scale(0.9f)
                        .intoMenuItem([this] {
                            auto newdu = currentDuration + Duration::fromHours(1);
                            this->setDuration(newdu);
                        })
                )
                .updateLayout()
        )
        .updateLayout();

    durRoot->updateLayout();

    // background for durRoot
    Build<CCScale9Sprite>::create("square02_001.png", CCRect{0.f, 0.f, 80.f, 80.f})
        .opacity(75)
        .zOrder(-5)
        .id("bg")
        .parent(durRoot)
        .contentSize(durRoot->getScaledContentSize() * 0.975f)
        .pos(durRoot->getScaledContentSize() / 2.f)
        .anchorPoint(0.5f, 0.5f);

    // Buttons for applying or removing punishment

    auto* menu = Build<CCMenu>::create()
        .layout(RowLayout::create()->setAutoScale(true))
        .parent(rootLayout)
        .collect();


    Build<ButtonSprite>::create("Submit", "bigFont.fnt", "GJ_button_01.png", 0.8f)
        .scale(0.9f)
        .intoMenuItem([this] {
            this->submit();
        })
        .parent(menu);

    if (punishment) {
        Build<ButtonSprite>::create(isBan ? "Unban" : "Unmute", "bigFont.fnt", "GJ_button_01.png", 0.8f)
            .scale(0.9f)
            .intoMenuItem([this] {
                this->submitRemoval();
            })
            .parent(menu);
    }

    menu->updateLayout();

    rootLayout->updateLayout();

    // if the user already was punished, set the data
    if (punishment) {
        auto expiry = SystemTime::fromUnix(punishment->expiresAt);

        if (punishment->expiresAt == 0 || expiry.isPast()) {
            this->setDuration(Duration{});
        } else {
            this->setDuration((expiry - SystemTime::now()).value_or(Duration{}));
        }

        this->setReason(punishment->reason);
    }

    return true;
}

void AdminPunishUserPopup::setDuration(Duration dur, bool inCallback) {
    if (dur > Duration::fromYears(100)) {
        dur = Duration{};
    }

    currentDuration = dur;

    for (const auto& [key, btn] : durationButtons) {
        if (key != dur) {
            btn->toggle(false);
        }
    }

    if (!inCallback) {
        if (durationButtons.contains(dur)) {
            durationButtons.at(dur)->toggle(true);
        }
    }

    auto cdays = currentDuration.days();
    auto chours = currentDuration.hours() % 24;

    daysInput->setString(fmt::format("{}", cdays));
    hoursInput->setString(fmt::format("{}", chours));
}

void AdminPunishUserPopup::setReason(const std::string& reason) {
    reasonInput->setString(reason);
}

void AdminPunishUserPopup::inputChanged() {
    auto cdays = util::format::parse<uint32_t>(daysInput->getString()).value_or(0);
    auto chours = util::format::parse<uint32_t>(hoursInput->getString()).value_or(0);

    // i hate c++ im going to rewrite the std lib one day
    // update 1 day later: i did :) this no longer uses std::chrono

    this->setDuration(Duration::fromDays(cdays) + Duration::fromHours(chours));
}

void AdminPunishUserPopup::submit() {
    auto expiresAtTime = SystemTime::now() + currentDuration;
    time_t expiresAt; // seconds since unix epoch

    if (currentDuration.isZero() || expiresAtTime.isPast()) {
        // permanent punishment
        expiresAt = 0;
    } else {
        expiresAt = expiresAtTime.to_time_t();
    }

    auto reason = this->reasonInput->getString();

    // if not editing a punishment, submit a new one
    std::shared_ptr<Packet> pkt;

    if (!punishment) {
        pkt = AdminPunishUserPacket::create(accountId, isBan, reason, expiresAt);
    } else {
        // edit the punishment otherwise
        pkt = AdminEditPunishmentPacket::create(accountId, isBan, reason, expiresAt);
    }

    this->parentPopup->performAction(std::move(pkt));
    this->onClose(nullptr);
}

void AdminPunishUserPopup::submitRemoval() {
    this->parentPopup->performAction(AdminRemovePunishmentPacket::create(accountId, isBan));
    this->onClose(nullptr);
}

AdminPunishUserPopup* AdminPunishUserPopup::create(AdminUserPopup* popup, int32_t accountId, bool isBan, std::optional<UserPunishment> punishment) {
    auto ret = new AdminPunishUserPopup;
    if (ret->initAnchored(350.f, 250.f, popup, accountId, isBan, std::move(punishment))) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}
