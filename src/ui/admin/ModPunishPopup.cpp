#include "ModPunishPopup.hpp"
#include <globed/core/PopupManager.hpp>
#include <core/net/NetworkManagerImpl.hpp>

#include <UIBuilder.hpp>
#include <asp/time/SystemTime.hpp>

using namespace geode::prelude;
using namespace asp::time;

namespace globed {

const CCSize ModPunishPopup::POPUP_SIZE { 350.f, 250.f };

bool ModPunishPopup::setup(int accountId, UserPunishmentType type, std::optional<UserPunishment> pun) {
    switch (type) {
        case UserPunishmentType::Ban: this->setTitle("Ban/Unban user");
        case UserPunishmentType::RoomBan: this->setTitle("Room ban/unban user");
        case UserPunishmentType::Mute: this->setTitle("Mute/Unmute user");
    }

    m_punishment = std::move(pun);
    m_accountId = accountId;
    m_type = type;

    auto* rootLayout = Build<CCNode>::create()
        .contentSize(m_size.width * 0.9f, m_size.height * 0.7f)
        .pos(this->fromCenter(0.f, -20.f))
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
    m_reasonInput = Build<TextInput>::create(reasonLayout->getContentWidth() * 0.75f, "Reason", "chatFont.fnt")
        .with([&](TextInput* input) {
            input->setCommonFilter(CommonFilter::Any);
        })
        .parent(reasonLayout);

    Build<CCSprite>::createSpriteName("GJ_closeBtn_001.png")
        .scale(0.68f)
        .intoMenuItem([this] {
            m_reasonInput->setString("");
        })
        .parent(reasonLayout);

    Build<CCSprite>::createSpriteName("btn_chatHistory_001.png")
        .intoMenuItem([this, type] {
            // CommonReasonPopup::create(this, type)->show();
            // TODO: common reasons by server
        })
        .parent(reasonLayout);

    reasonLayout->updateLayout();

    Build<CCLabelBMFont>::create(type != UserPunishmentType::Mute ? "Ban Duration" : "Mute Duration", "bigFont.fnt")
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
        m_durationButtons.try_emplace(off, toggler);
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
                            auto newdu = m_currentDuration - Duration::fromDays(1);

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
                        .store(m_daysInput)
                )
                .child(
                    Build<CCSprite>::createSpriteName("edit_rightBtn_001.png")
                        .scale(0.9f)
                        .intoMenuItem([this] {
                            auto newdu = m_currentDuration + Duration::fromDays(1);
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
                            auto newdu = m_currentDuration - Duration::fromHours(1);
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
                        .store(m_hoursInput)
                )
                .child(
                    Build<CCSprite>::createSpriteName("edit_rightBtn_001.png")
                        .scale(0.9f)
                        .intoMenuItem([this] {
                            auto newdu = m_currentDuration + Duration::fromHours(1);
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

    if (m_punishment) {
        Build<ButtonSprite>::create(type != UserPunishmentType::Mute ? "Unban" : "Unmute", "bigFont.fnt", "GJ_button_01.png", 0.8f)
            .scale(0.9f)
            .intoMenuItem([this] {
                this->submitRemoval();
            })
            .parent(menu);
    }

    menu->updateLayout();

    rootLayout->updateLayout();

    // if the user already was punished, set the data
    if (m_punishment) {
        auto expiry = SystemTime::fromUnix(m_punishment->expiresAt);

        if (m_punishment->expiresAt == 0 || expiry.isPast()) {
            this->setDuration(Duration{});
        } else {
            this->setDuration((expiry - SystemTime::now()).value_or(Duration{}));
        }

        this->setReason(m_punishment->reason);
    } else {
        // default to 1 day
        this->setDuration(Duration::fromDays(1));
    }

    return true;
}

void ModPunishPopup::setDuration(asp::time::Duration dur, bool inCallback) {
    if (dur > Duration::fromYears(100)) {
        dur = Duration{};
    }

    m_currentDuration = dur;

    for (const auto& [key, btn] : m_durationButtons) {
        if (key != dur) {
            btn->toggle(false);
        }
    }

    if (!inCallback) {
        if (m_durationButtons.contains(dur)) {
            m_durationButtons.at(dur)->toggle(true);
        }
    }

    auto cdays = m_currentDuration.days();
    auto chours = m_currentDuration.hours() % 24;

    m_daysInput->setString(fmt::format("{}", cdays));
    m_hoursInput->setString(fmt::format("{}", chours));
}

void ModPunishPopup::setCallback(Callback&& cb) {
    m_callback = std::move(cb);
}

void ModPunishPopup::setReason(const std::string& reason) {
    m_reasonInput->setString(reason);
}

void ModPunishPopup::inputChanged() {
    auto cdays = utils::numFromString<uint32_t>(m_daysInput->getString()).unwrapOr(0);
    auto chours = utils::numFromString<uint32_t>(m_hoursInput->getString()).unwrapOr(0);

    // i hate c++ im going to rewrite the std lib one day
    // update 1 day later: i did :) this no longer uses std::chrono

    this->setDuration(Duration::fromDays(cdays) + Duration::fromHours(chours));
}

void ModPunishPopup::submit() {
    this->startWaiting();

    auto expiresAtTime = SystemTime::now() + m_currentDuration;
    time_t expiresAt; // seconds since unix epoch

    if (m_currentDuration.isZero() || expiresAtTime.isPast()) {
        // permanent punishment
        expiresAt = 0;
    } else {
        expiresAt = expiresAtTime.to_time_t();
    }

    auto reason = m_reasonInput->getString();

    auto& nm = NetworkManagerImpl::get();

    switch (m_type) {
        case UserPunishmentType::Ban: {
            nm.sendAdminBan(m_accountId, reason, expiresAt);
        } break;

        case UserPunishmentType::Mute: {
            nm.sendAdminMute(m_accountId, reason, expiresAt);
        } break;

        case UserPunishmentType::RoomBan: {
            nm.sendAdminRoomBan(m_accountId, reason, expiresAt);
        } break;
    }
}

void ModPunishPopup::submitRemoval() {
    this->startWaiting();

    auto& nm = NetworkManagerImpl::get();

    switch (m_type) {
        case UserPunishmentType::Ban: {
            nm.sendAdminUnban(m_accountId);
        } break;

        case UserPunishmentType::Mute: {
            nm.sendAdminUnmute(m_accountId);
        } break;

        case UserPunishmentType::RoomBan: {
            nm.sendAdminRoomUnban(m_accountId);
        } break;
    }
}

void ModPunishPopup::startWaiting() {
    m_listener = NetworkManagerImpl::get().listen<msg::AdminResultMessage>([this](const auto& msg) {
        this->stopWaiting(msg);
        return ListenerResult::Stop;
    });
    m_listener->setPriority(-100);

    if (m_loadPopup) {
        m_loadPopup->forceClose();
    }

    m_loadPopup = LoadingPopup::create();
    m_loadPopup->show();
}

void ModPunishPopup::stopWaiting(const msg::AdminResultMessage& msg) {
    m_listener.reset();

    if (m_loadPopup) {
        m_loadPopup->forceClose();
        m_loadPopup = nullptr;
    }

    if (!msg.success) {
        globed::alertFormat("Error", "Failed to punish user: <cy>{}</c>", msg.error);
    }

    if (m_callback) m_callback();

    this->onClose(nullptr);
}

}