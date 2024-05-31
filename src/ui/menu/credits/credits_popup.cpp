#include "credits_popup.hpp"
#include <matjson.hpp>
#include <util/ui.hpp>

#include "credits_player.hpp"
#include "credits_cell.hpp"

#include <Geode/utils/web.hpp>

using namespace geode::prelude;

bool GlobedCreditsPopup::setup() {
    using Icons = GlobedSimplePlayer::Icons;

    this->setTitle("Credits");

    auto rlayout = util::ui::getPopupLayout(m_size);

    auto* listlayer = Build<GJCommentListLayer>::create(nullptr, "", util::ui::BG_COLOR_BROWN, LIST_WIDTH, LIST_HEIGHT, false)
        .pos((m_mainLayer->getScaledContentSize().width - LIST_WIDTH) / 2, 60.f)
        .parent(m_mainLayer)
        .collect();

    auto* scrollLayer = Build(ScrollLayer::create({LIST_WIDTH, LIST_HEIGHT}))
        .parent(listlayer)
        .collect();

    scrollLayer->m_contentLayer->setLayout(
        ColumnLayout::create()
            ->setGap(0.f)
            ->setAxisReverse(true)
            ->setAxisAlignment(AxisAlignment::End)
            ->setAutoScale(false)
    );

#define ADD_PLAYER(array, name, accountid, userid, cube, col1, col2, col3) \
    array->addObject( \
        GlobedCreditsPlayer::create(name, name, accountid, userid, Icons { .type = IconType::Cube, .id = cube, .color1 = col1, .color2 = col2, .color3 = col3}) \
    )
#define ADD_PLAYER_NICK(array, name, nickname, accountid, userid, cube, col1, col2, col3) \
    array->addObject( \
        GlobedCreditsPlayer::create(name, nickname, accountid, userid, Icons { .type = IconType::Cube, .id = cube, .color1 = col1, .color2 = col2, .color3 = col3}) \
    )

    /* Fetch credits from server */

    web::AsyncWebRequest()
        .fetch("http://127.0.0.1:8080/credits")
        .json()
        .then([scrollLayer](matjson::Value const& credits_data) {
            /* Owners */

            CCArray* owners = CCArray::create();
            auto owner_credits = credits_data["owner"].as_array();

            for (auto& credit : owner_credits) {
                ADD_PLAYER_NICK(
                    owners,
                    credit["name"].as_string(),
                    credit["name"].as_string(),
                    credit["accountID"].as_int(),
                    credit["userID"].as_int(),
                    credit["iconID"].as_int(),
                    credit["color1"].as_int(),
                    credit["color2"].as_int(),
                    credit["color3"].as_int());
            }

            auto* cellOwners = Build<GlobedCreditsCell>::create("Owner", false, owners)
                .id("cell-owner"_spr)
                .parent(scrollLayer->m_contentLayer)
                .collect();

            /* Developer */

            CCArray* developers = CCArray::create();
            auto dev_credits = credits_data["dev"].as_array();

            for (auto& credit : dev_credits) {
                ADD_PLAYER_NICK(
                    developers,
                    credit["name"].as_string(),
                    credit["name"].as_string(),
                    credit["accountID"].as_int(),
                    credit["userID"].as_int(),
                    credit["iconID"].as_int(),
                    credit["color1"].as_int(),
                    credit["color2"].as_int(),
                    credit["color3"].as_int());
            }

            auto* cellDevs = Build<GlobedCreditsCell>::create("Developer", true, developers)
                .id("cell-developer"_spr)
                .parent(scrollLayer->m_contentLayer)
                .collect();            

            /* Staff */

            CCArray* staff = CCArray::create();
            auto staff_credits = credits_data["staff"].as_array();

            for (auto& credit : staff_credits) {
                ADD_PLAYER_NICK(
                    staff,
                    credit["name"].as_string(),
                    credit["name"].as_string(),
                    credit["accountID"].as_int(),
                    credit["userID"].as_int(),
                    credit["iconID"].as_int(),
                    credit["color1"].as_int(),
                    credit["color2"].as_int(),
                    credit["color3"].as_int());
            }

            auto* cellStaff = Build<GlobedCreditsCell>::create("Staff", false, staff)
                .id("cell-staff"_spr)
                .parent(scrollLayer->m_contentLayer)
                .collect();

            /* Contributor */

            CCArray* contributors = CCArray::create();

            auto contrib_credits = credits_data["contrib"].as_array();

            for (auto& credit : contrib_credits) {
                ADD_PLAYER_NICK(
                    contributors,
                    credit["name"].as_string(),
                    credit["name"].as_string(),
                    credit["accountID"].as_int(),
                    credit["userID"].as_int(),
                    credit["iconID"].as_int(),
                    credit["color1"].as_int(),
                    credit["color2"].as_int(),
                    credit["color3"].as_int());
            }

            auto* cellContrib = Build<GlobedCreditsCell>::create("Contributor", true, contributors)
                .id("cell-contributor"_spr)
                .parent(scrollLayer->m_contentLayer)
                .collect();

            /* Special */

            CCArray* special = CCArray::create();

            auto special_credits = credits_data["special"].as_array();

            for (auto& credit : special_credits) {
                ADD_PLAYER_NICK(
                    special,
                    credit["name"].as_string(),
                    credit["name"].as_string(),
                    credit["accountID"].as_int(),
                    credit["userID"].as_int(),
                    credit["iconID"].as_int(),
                    credit["color1"].as_int(),
                    credit["color2"].as_int(),
                    credit["color3"].as_int());
            }

            auto* cellSpecial = Build<GlobedCreditsCell>::create("Special thanks", false, special)
                .id("cell-special"_spr)
                .parent(scrollLayer->m_contentLayer)
                .collect();

            scrollLayer->m_contentLayer->setContentHeight(
                cellOwners->getScaledContentSize().height
                + cellDevs->getScaledContentSize().height
                + cellStaff->getScaledContentSize().height
                + cellContrib->getScaledContentSize().height
                + cellSpecial->getScaledContentSize().height
            );
            scrollLayer->m_contentLayer->updateLayout();

            // make the list start at the top instead of the bottom
            util::ui::scrollToTop(scrollLayer);
        })
        .expect([](std::string const& error) {
            FLAlertLayer::create("Failed to get credits", fmt::format("{}", error), "OK")->show();
        });

    return true;
}

GlobedCreditsPopup* GlobedCreditsPopup::create() {
    auto ret = new GlobedCreditsPopup;
    if (ret->init(POPUP_WIDTH, POPUP_HEIGHT)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}