#include "credits_popup.hpp"
#include <matjson.hpp>
#include <matjson/stl_serialize.hpp>
#include <util/ui.hpp>

#include "credits_player.hpp"
#include "credits_cell.hpp"

#include <Geode/utils/web.hpp>

using namespace geode::prelude;

struct CreditsUser {
    std::string name, gameName;
    int accountId, userId;
    int color1, color2, color3;
    int iconId;
};

template <>
struct matjson::Serialize<CreditsUser> {
    static CreditsUser from_json(const matjson::Value& value) {
        return CreditsUser {
            .name = value["name"].as_string(),
            .gameName = value["gameName"].as_string(),
            .accountId = value["accountID"].as_int(),
            .userId = value["userID"].as_int(),
            .color1 = value["color1"].as_int(),
            .color2 = value["color2"].as_int(),
            .color3 = value["color3"].as_int(),
            .iconId = value["iconID"].as_int()
        };
    }

    static matjson::Value to_json(const CreditsUser& user) {
        throw std::runtime_error("unimplemented");
    }

    static bool is_json(const matjson::Value& value) {
        if (
            !value.contains("name")
            || !value.contains("gameName")
            || !value.contains("accountID")
            || !value.contains("userID")
            || !value.contains("color1")
            || !value.contains("color2")
            || !value.contains("color3")
            || !value.contains("iconID")
        ) {
            return false;
        }

        return value["name"].is_string()
            && value["gameName"].is_string()
            && value["accountID"].is_number()
            && value["userID"].is_number()
            && value["color1"].is_number()
            && value["color2"].is_number()
            && value["color3"].is_number()
            && value["iconID"].is_number()
        ;
    }
};

struct CreditsResponse {
    std::vector<CreditsUser> owner, staff, contributors, special;
};

template <>
struct matjson::Serialize<CreditsResponse> {
    static CreditsResponse from_json(const matjson::Value& value) {
        auto owner = value["owner"].as<std::vector<CreditsUser>>();
        auto staff = value["staff"].as<std::vector<CreditsUser>>();
        auto contributors = value["contributors"].as<std::vector<CreditsUser>>();
        auto special = value["special"].as<std::vector<CreditsUser>>();

        return CreditsResponse {
            .owner = owner,
            .staff = staff,
            .contributors = contributors,
            .special = special
        };
    }

    static bool is_json(const matjson::Value& value) {
        if (!value.contains("owner") || !value.contains("staff") || !value.contains("contributors") || !value.contains("special")) return false;

        auto isValid = [](const matjson::Value& value) -> bool {
            if (!value.is_array()) return false;

            for (auto& elem : value.as_array()) {
                if (!elem.is<CreditsUser>()) return false;
            }

            return true;
        };

        return
            isValid(value["owner"])
            && isValid(value["staff"])
            && isValid(value["contributors"])
            && isValid(value["special"]);
    }
};

bool GlobedCreditsPopup::setup() {
    using Icons = GlobedSimplePlayer::Icons;

    this->setTitle("Credits");

    auto rlayout = util::ui::getPopupLayout(m_size);

    auto* listlayer = Build<GJCommentListLayer>::create(nullptr, "", util::ui::BG_COLOR_BROWN, LIST_WIDTH, LIST_HEIGHT, false)
        .pos((m_mainLayer->getScaledContentSize().width - LIST_WIDTH) / 2, 60.f)
        .parent(m_mainLayer)
        .collect();

    Build(ScrollLayer::create({LIST_WIDTH, LIST_HEIGHT}))
        .parent(listlayer)
        .store(scrollLayer)
        .collect();

    scrollLayer->m_contentLayer->setLayout(
        ColumnLayout::create()
            ->setGap(0.f)
            ->setAxisReverse(true)
            ->setAxisAlignment(AxisAlignment::End)
            ->setAutoScale(false)
    );

#define ADD_PLAYER(array, obj) \
    array->addObject( \
        GlobedCreditsPlayer::create(obj.gameName, obj.name, obj.accountId, obj.userId, \
            GlobedSimplePlayer::Icons { .type = IconType::Cube, .id = obj.iconId, .color1 = obj.color1, .color2 = obj.color2, .color3 = obj.color3}) \
    )

    /* Fetch credits from server */

    auto task = web::WebRequest()
        // .get("http://credits.globed.dev/credits")
        .get("http://127.0.0.1:8080/credits")
        .map([](web::WebResponse* response) -> Result<std::string, std::string> {
            GLOBED_UNWRAP_INTO(response->string(), auto resp);

            if (response->ok()) {
                return Ok(resp);
            } else {
                return Err(resp);
            }
        }, [](auto) -> std::monostate {
            return {};
        });

    eventListener.bind(this, &GlobedCreditsPopup::requestCallback);
    eventListener.setFilter(task);

    return true;
}

void GlobedCreditsPopup::requestCallback(Task<Result<std::string, std::string>>::Event* e) {
    if (!e || !e->getValue()) return;

    auto result = e->getValue();
    if (result->isErr()) {
        FLAlertLayer::create("Error", fmt::format("Failed to load credits.\n\nReason: <cy>{}</c>", result->unwrapErr()), "Ok")->show();
        return;
    }

    auto response = result->unwrap();

    std::string parseError;
    auto creditsDataOpt = matjson::parse(response, parseError);

    if (creditsDataOpt && !creditsDataOpt->is<CreditsResponse>()) {
        parseError = "invalid json response";
    }

    if (!creditsDataOpt || !parseError.empty()) {
        FLAlertLayer::create("Error", fmt::format("Failed to parse credits response.\n\nReason: <cy>{}</c>", parseError), "Ok")->show();
        return;
    }

    auto creditsData = creditsDataOpt.value().as<CreditsResponse>();

    /* Owner */

    CCArray* owners = CCArray::create();

    for (auto& credit : creditsData.owner) {
        ADD_PLAYER(owners, credit);
    }

    auto* cellOwners = Build<GlobedCreditsCell>::create("Owner", false, owners)
        .id("cell-owner"_spr)
        .parent(scrollLayer->m_contentLayer)
        .collect();

    /* Staff */

    CCArray* staff = CCArray::create();

    for (auto& credit : creditsData.staff) {
        ADD_PLAYER(staff, credit);
    }

    auto* cellStaff = Build<GlobedCreditsCell>::create("Staff", true, staff)
        .id("cell-staff"_spr)
        .parent(scrollLayer->m_contentLayer)
        .collect();

    /* Contributor */

    CCArray* contributors = CCArray::create();

    for (auto& credit : creditsData.contributors) {
        ADD_PLAYER(contributors, credit);
    }

    auto* cellContributors = Build<GlobedCreditsCell>::create("Contributor", false, contributors)
        .id("cell-contributor"_spr)
        .parent(scrollLayer->m_contentLayer)
        .collect();

    /* Special */

    CCArray* special = CCArray::create();

    for (auto& credit : creditsData.special) {
        ADD_PLAYER(special, credit);
    }

    auto* cellSpecial = Build<GlobedCreditsCell>::create("Special thanks", true, special)
        .id("cell-special"_spr)
        .parent(scrollLayer->m_contentLayer)
        .collect();

    scrollLayer->m_contentLayer->setContentHeight(
        cellOwners->getScaledContentSize().height
        + cellStaff->getScaledContentSize().height
        + cellContributors->getScaledContentSize().height
        + cellSpecial->getScaledContentSize().height
    );
    scrollLayer->m_contentLayer->updateLayout();

    // make the list start at the top instead of the bottom
    util::ui::scrollToTop(scrollLayer);
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