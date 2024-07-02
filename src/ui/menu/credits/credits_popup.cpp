#include "credits_popup.hpp"

#include <matjson.hpp>
#include <matjson/stl_serialize.hpp>

#include "credits_player.hpp"
#include <managers/error_queues.hpp>
#include <util/ui.hpp>

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

static std::optional<CreditsResponse> g_cachedResponse;

bool GlobedCreditsPopup::setup() {
    using Icons = GlobedSimplePlayer::Icons;

    this->setTitle("Credits");
    this->setID("GlobedCreditsPopup"_spr);

    auto rlayout = util::ui::getPopupLayout(m_size);

    Build(CreditsList::createForComments(LIST_WIDTH, LIST_HEIGHT, 0.f))
        .anchorPoint(0.5f, 1.f)
        .pos(rlayout.fromTop(40.f))
        .store(listLayer)
        .parent(m_mainLayer);

    // if not cached, make a request
    if (!g_cachedResponse) {
        auto& wrm = WebRequestManager::get();
        eventListener.bind(this, &GlobedCreditsPopup::requestCallback);
        eventListener.setFilter(wrm.fetchCredits());
    } else {
        // otherwise use the cached response
        this->setupFromCache();
    }

    return true;
}

void GlobedCreditsPopup::requestCallback(WebRequestManager::Task::Event* e) {
    if (!e || !e->getValue()) return;

    auto result = e->getValue();
    if (result->isErr()) {
        auto err = result->unwrapErr();
        ErrorQueues::get().error(fmt::format("Failed to load credits.\n\nReason: <cy>{}</c> (code {})", err.message, err.code));
        return;
    }

    auto response = result->unwrap();
    if (response == "{}") {
        FLAlertLayer::create("Error", "<cy>Credits currently unavailable, please try again later.</c>", "Ok")->show();
        return;
    }

    std::string parseError;
    auto creditsDataOpt = matjson::parse(response, parseError);

    if (creditsDataOpt && !creditsDataOpt->is<CreditsResponse>()) {
        parseError = "invalid json response";
    }

    if (!creditsDataOpt || !parseError.empty()) {
        FLAlertLayer::create("Error", fmt::format("Failed to parse credits response.\n\nReason: <cy>{}</c>", parseError), "Ok")->show();
        return;
    }

    g_cachedResponse = creditsDataOpt.value().as<CreditsResponse>();
    this->setupFromCache();
}

void GlobedCreditsPopup::setupFromCache() {
    GLOBED_REQUIRE(g_cachedResponse, "setupFromCache called when no cached response is available");

#define ADD_PLAYER(array, obj) \
    array->addObject( \
        GlobedCreditsPlayer::create(obj.gameName, obj.name, obj.accountId, obj.userId, \
            GlobedSimplePlayer::Icons(obj.iconId, obj.color1, obj.color2, obj.color3)) \
    )

    /* Owner */

    CCArray* owners = CCArray::create();

    for (auto& credit : g_cachedResponse->owner) {
        ADD_PLAYER(owners, credit);
    }

    listLayer->addCell("Owner", false, owners);

    /* Staff */

    CCArray* staff = CCArray::create();

    for (auto& credit : g_cachedResponse->staff) {
        ADD_PLAYER(staff, credit);
    }

    listLayer->addCell("Staff", false, staff);

    /* Contributor */

    CCArray* contributors = CCArray::create();

    for (auto& credit : g_cachedResponse->contributors) {
        ADD_PLAYER(contributors, credit);
    }

    listLayer->addCell("Contributor", false, contributors);

    /* Special */

    CCArray* special = CCArray::create();

    for (auto& credit : g_cachedResponse->special) {
        ADD_PLAYER(special, credit);
    }

    listLayer->addCell("Special thanks", false, special);

    // make the list start at the top instead of the bottom
    listLayer->scrollToTop();
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
