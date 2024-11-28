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
    static Result<CreditsUser> fromJson(const matjson::Value& value) {
        if (!(value["name"].isString()
            && value["gameName"].isString()
            && value["accountID"].isNumber()
            && value["userID"].isNumber()
            && value["color1"].isNumber()
            && value["color2"].isNumber()
            && value["color3"].isNumber()
            && value["iconID"].isNumber()))

        {
            return Err("invalid type");
        }

        return Ok(CreditsUser {
            .name = value["name"].asString().unwrapOrDefault(),
            .gameName = value["gameName"].asString().unwrapOrDefault(),
            .accountId = (int) value["accountID"].asInt().unwrapOrDefault(),
            .userId = (int) value["userID"].asInt().unwrapOrDefault(),
            .color1 = (int) value["color1"].asInt().unwrapOrDefault(),
            .color2 = (int) value["color2"].asInt().unwrapOrDefault(),
            .color3 = (int) value["color3"].asInt().unwrapOrDefault(),
            .iconId = (int) value["iconID"].asInt().unwrapOrDefault()
        });
    }

    static matjson::Value toJson(const CreditsUser& user) {
        throw std::runtime_error("unimplemented");
    }
};

struct CreditsResponse {
    std::vector<CreditsUser> owner, staff, contributors, special;
};

template <>
struct matjson::Serialize<CreditsResponse> {
    static Result<CreditsResponse> fromJson(const matjson::Value& value) {
        auto owner = value["owner"].as<std::vector<CreditsUser>>();
        auto staff = value["staff"].as<std::vector<CreditsUser>>();
        auto contributors = value["contributors"].as<std::vector<CreditsUser>>();
        auto special = value["special"].as<std::vector<CreditsUser>>();

        if (!owner || !staff || !contributors || !special) {
            return Err("invalid type");
        }

        return Ok(CreditsResponse {
            .owner = owner.unwrapOrDefault(),
            .staff = staff.unwrapOrDefault(),
            .contributors = contributors.unwrapOrDefault(),
            .special = special.unwrapOrDefault()
        });
    }
};

static std::optional<CreditsResponse> g_cachedResponse;

bool GlobedCreditsPopup::setup() {
    using Icons = GlobedSimplePlayer::Icons;

    this->setTitle("Credits");
    this->setID("GlobedCreditsPopup"_spr);

    auto rlayout = util::ui::getPopupLayoutAnchored(m_size);

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

    static auto retError = [](auto msg) {
        ErrorQueues::get().error(fmt::format("Failed to load credits.\n\nReason: <cy>{}</c>", msg));
    };

    auto result = e->getValue();
    if (!result->ok()) {
        retError(result->getError());
        return;
    }

    auto creditsDataOpt = result->json();
    if (!creditsDataOpt) {
        retError(fmt::format("JSON error: {}", creditsDataOpt.unwrapErr()));
        return;
    }

    auto creditsData = creditsDataOpt.unwrap();

    if (creditsData.size() == 0) {
        retError("Empty response object returned");
        return;
    }

    auto ccresp = creditsData.as<CreditsResponse>();
    if (!ccresp) {
        retError("invalid json response");
        return;
    }

    g_cachedResponse = ccresp.unwrap();
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
    if (ret->initAnchored(POPUP_WIDTH, POPUP_HEIGHT)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}
