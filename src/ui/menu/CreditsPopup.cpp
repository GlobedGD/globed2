#include "CreditsPopup.hpp"
#include <globed/core/PopupManager.hpp>
#include <globed/core/actions.hpp>
#include <core/net/NetworkManagerImpl.hpp>

#include <UIBuilder.hpp>
#include <cue/PlayerIcon.hpp>

using namespace geode::prelude;

namespace globed {

const CCSize CreditsPopup::POPUP_SIZE {380.f, 260.f };
static constexpr CCSize LIST_SIZE { 340.f, 200.f};

// TODO: credits data
static const std::vector<CreditsCategory>& defaultData();

namespace {
class CreditsPlayerNode : public CCNode {
public:
    static CreditsPlayerNode* create(const CreditsUser& user) {
        auto ret = new CreditsPlayerNode;
        if (ret->init(user)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }

private:
    bool init(const CreditsUser& user) {
        CCNode::init();

        auto* sp = Build(cue::PlayerIcon::create(cue::Icons {
            .type = IconType::Cube,
            .id = user.cube,
            .color1 = user.color1.asIdx(),
            .color2 = user.color2.asIdx(),
            .glowColor = user.glowColor.asIdx(),
        }))
            .anchorPoint({0.5f, 0.5f})
            .zOrder(0)
            .parent(this)
            .collect();

        // shadow
        auto* shadow = Build<CCSprite>::create("shadow-thing-idk.png"_spr)
            .scale(0.55f)
            .pos(0.f, -13.f)
            .zOrder(-1)
            .parent(this)
            .collect();

        // name
        CCMenuItemSpriteExtra* nameLabel;
        auto menu = Build<CCLabelBMFont>::create(user.displayName.c_str(), "goldFont.fnt")
            .scale(0.45f)
            .limitLabelWidth(52.f, 0.45f, 0.05f)
            .intoMenuItem([accountId = user.accountId, userId = user.userId, name = user.username] {
                openUserProfile(accountId, userId, name);
            })
            .pos(0.f, 22.f)
            .store(nameLabel)
            .intoNewParent(CCMenu::create())
            .pos(0.f, 0.f)
            .parent(this)
            .collect();

        float width = sp->getScaledContentSize().width * 1.1f;
        float height = sp->getScaledContentSize().height * 1.1f + nameLabel->getScaledContentSize().height;

        this->setContentSize({width, height});

        CCPoint delta = CCPoint{width, height} / 2;
        sp->setPosition(sp->getPosition() + delta);
        shadow->setPosition(shadow->getPosition() + delta);
        menu->setPosition(menu->getPosition() + delta);

        return true;
    }

};

class CategoryCell : public CCNode {
public:
    static CategoryCell* create(const globed::CreditsCategory& cat) {
        auto ret = new CategoryCell;
        if (ret->init(cat)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }

private:
    bool init(const globed::CreditsCategory& cat) {
        CCNode::init();

        constexpr size_t PLAYERS_IN_ROW = 6;

        size_t rows = (cat.users.size() + PLAYERS_IN_ROW - 1) / PLAYERS_IN_ROW;

        auto* title = Build<CCLabelBMFont>::create(cat.name.c_str(), "bigFont.fnt")
            .scale(0.68f)
            .pos(LIST_SIZE.width / 2, 3.f)
            .parent(this)
            .collect();

        const float wrapperGap = 8.f;
        auto* playerWrapper = Build<CCNode>::create()
            .layout(ColumnLayout::create()->setAxisReverse(true)->setGap(wrapperGap))
            .pos(LIST_SIZE.width / 2.f, 0.f)
            .anchorPoint(0.5f, 0.0f)
            .contentSize(LIST_SIZE.width, 50.f)
            .id("player-wrapper"_spr)
            .parent(this)
            .collect();

        float wrapperHeight = 0.f;

        for (size_t i = 0; i < rows; i++) {
            size_t firstIdx = i * PLAYERS_IN_ROW;
            size_t lastIdx = std::min((i + 1) * PLAYERS_IN_ROW, cat.users.size());
            size_t count = lastIdx - firstIdx;

            float playerGap = 15.f;
            if (count < 4) {
                playerGap = 40.f;
            } else if (count == 4) {
                playerGap = 30.f;
            } else if (count == 5) {
                playerGap = 25.f;
            } else if (count == 6) {
                playerGap = 18.f;
            }

            auto* row = Build<CCNode>::create()
                .layout(RowLayout::create()->setGap(playerGap))
                .id("wrapper-row"_spr)
                .parent(playerWrapper)
                .contentSize(LIST_SIZE.width, 0.f)
                .collect();

            for (size_t i = firstIdx; i < lastIdx; i++) {
                row->addChild(CreditsPlayerNode::create(cat.users[i]));
            }

            row->updateLayout();

            wrapperHeight += row->getContentHeight();
            if (i != 0) {
                wrapperHeight += wrapperGap;
            }
        }

        playerWrapper->setContentSize({0.f, wrapperHeight});
        playerWrapper->updateLayout();

        this->setContentSize(CCSize{LIST_SIZE.width, playerWrapper->getScaledContentSize().height + 8.f + title->getScaledContentSize().height});

        // title at the top
        title->setPosition({title->getPositionX(), this->getContentHeight() - 10.f});

        return true;
    }
};
}

bool CreditsPopup::setup() {
    this->setTitle("Credits");

    m_list = Build(cue::ListNode::create(LIST_SIZE))
        .pos(this->fromCenter(0.f, -10.f))
        .parent(m_mainLayer);

    m_list->setAutoUpdate(false);

    auto& nm = NetworkManagerImpl::get();
    m_listener = nm.listen<msg::CreditsMessage>([this](const auto& msg) {
        if (msg.unavailable) {
            globed::alert("Error", "Credits are temporarily unavailable.");
        } else {
            this->onLoaded(msg.categories);
        }

        return ListenerResult::Stop;
    });

    if (nm.isConnected()) {
        nm.sendFetchCredits();
    } else {
        this->onLoaded(defaultData());
    }

    return true;
}

void CreditsPopup::onLoaded(const std::vector<CreditsCategory>& categories) {
    m_list->clear();

    for (auto& cat : categories) {
        if (cat.users.empty()) {
            continue;
        }

        m_list->addCell(CategoryCell::create(cat));
    }

    m_list->updateLayout();

    return;

    // this is useful for writing the static vector below
    std::string out;
    for (auto& cat : categories) {
        out += fmt::format("CreditsCategory {{ \"{}\", {{\n", cat.name);
        for (auto& user : cat.users) {
            out += fmt::format(
                "    CreditsUser {{ {}, {}, \"{}\", \"{}\", {}, {}, {}, {} }},\n",
                user.accountId,
                user.userId,
                user.username,
                user.displayName,
                user.cube,
                user.color1.asIdx(),
                user.color2.asIdx(),
                user.glowColor.asIdx()
            );
        }
        out += "}},\n";
    }
    log::info("\n{}", out);
}

static const std::vector<CreditsCategory>& defaultData() {
    static const std::vector<CreditsCategory> g_defaultData {
        CreditsCategory { "Owners", {
            CreditsUser { 1249399, 8533459, "nexter", "nexter", 461, 101, 62, 51 },
            CreditsUser { 9735891, 88588343, "dankmeme01", "dankmeme01", 1, 41, 16, -1 },
        }},
        CreditsCategory { "Admins", {
            CreditsUser { 2281975, 11275398, "Swishbue", "Swishbue", 132, 12, 41, 98 },
            CreditsUser { 6209270, 20531100, "eebysheeby", "eebysheeby", 272, 43, 12, -1 },
            CreditsUser { 8363104, 57276530, "Serpo", "Serpo", 459, 12, 19, 41 },
            CreditsUser { 10991940, 108555090, "Cutiny", "Cutiny", 30, 12, 43, 98 },
            CreditsUser { 26075508, 234267954, "NotPerfect5625", "NotPerfect5625", 483, 106, 63, -1 },
        }},
        CreditsCategory { "Moderators", {
            CreditsUser { 1199217, 5914812, "Nemira", "Nemira", 85, 41, 3, 3 },
            CreditsUser { 3167329, 5604085, "ImAkKo", "ImAkKo", 256, 11, 3, 3 },
            CreditsUser { 4569963, 15447500, "ItzKiba", "ItzKiba", 110, 6, 10, 42 },
            CreditsUser { 5722734, 19663714, "charlesmot", "charlesmot", 238, 3, 70, 3 },
            CreditsUser { 5906108, 20465032, "Jett James", "Jett James", 39, 42, 3, 83 },
            CreditsUser { 6879727, 38395219, "Szyly", "Szyly", 459, 12, 43, 43 },
            CreditsUser { 7731541, 55662104, "EpicLooch", "EpicLooch", 177, 17, 12, 3 },
            CreditsUser { 8262191, 61991331, "Jiflol", "Jiflol", 343, 44, 98, -1 },
            CreditsUser { 11979735, 120479236, "Jxbu", "Jxbu", 121, 18, 12, 11 },
            CreditsUser { 12016320, 40684062, "500skills", "500skills", 344, 44, 85, -1 },
            CreditsUser { 13488213, 119652180, "Starr44", "Starr44", 336, 52, 12, 98 },
            CreditsUser { 13640460, 127054504, "ciyaqil", "ciyaqil", 84, 12, 40, -1 },
            CreditsUser { 13678537, 132787232, "delivel", "delivel", 238, 12, 8, 51 },
            CreditsUser { 13807431, 127698632, "Quablue", "Quablue", 379, 12, 12, 12 },
            CreditsUser { 14794466, 150213998, "Antedrifter", "Antedrifter", 4, 12, 15, 12 },
            CreditsUser { 16045685, 158535626, "Lamnhiem", "Lamnhiem", 67, 12, 51, 12 },
            CreditsUser { 18298730, 169274462, "YESEvoi", "YESEvoi", 140, 3, 2, 12 },
            CreditsUser { 20284359, 179839933, "TechStudent10", "TechStudent10", 79, 10, 14, 29 },
            CreditsUser { 21143615, 190847322, "Xeronin", "Xeronin", 457, 18, 16, 12 },
            CreditsUser { 21975575, 94919182, "maurry", "maurry", 394, 41, 12, 12 },
            CreditsUser { 24387180, 216616532, "Goalers", "Goalers", 471, 94, 73, 12 },
            CreditsUser { 25155465, 224671815, "TaxiLad", "TaxiLad", 2, 12, 12, -1 },
            CreditsUser { 26619661, 238097046, "tTonk", "tTonk", 16, 71, 12, -1 },
        }},
        CreditsCategory { "Trial Moderators", {
        }},
        CreditsCategory { "Contributors", {
            CreditsUser { 1621348, 8463710, "availax", "availax", 1, 12, 52, 12 },
            CreditsUser { 7479054, 47290058, "ninXout", "ninXout", 77, 1, 5, -1 },
            CreditsUser { 20284359, 179839933, "TechStudent10", "TechStudent10", 79, 10, 14, 29 },
            CreditsUser { 16778880, 162245660, "TheSillyDoggo", "TheSillyDoggo", 98, 41, 12, 12 },
            CreditsUser { 28024715, 236381167, "angeld233", "angeld233", 98, 4, 12, 12 },
            CreditsUser { 25397826, 227796112, "Uproxide", "Uproxide", 296, 2, 12, 12 },
            CreditsUser { 4706010, 16077956, "NotTerma", "NotTerma", 379, 13, 8, 8 },
            CreditsUser { 7214334, 42454266, "LimeGradient", "LimeGradient", 46, 98, 12, 43 },
            CreditsUser { 4569963, 15447500, "ItzKiba", "ItzKiba", 110, 6, 10, 42 },
        }},
        CreditsCategory { "Special thanks", {
            CreditsUser { 1621348, 8463710, "availax", "availax", 1, 12, 52, 12 },
            CreditsUser { 3759035, 13781237, "MathieuAR", "MathieuAR", 220, 5, 9, 16 },
            CreditsUser { 104257, 1817908, "HJfod", "HJfod", 132, 8, 3, 40 },
            CreditsUser { 18950870, 172707109, "cozyton", "cozyton", 120, 13, 10, 98 },
            CreditsUser { 25517837, 153455117, "arixnr", "arixnr", 132, 52, 12, 12 },
            CreditsUser { 7689052, 53843371, "ArcticWoof", "ArcticWoof", 290, 83, 12, 3 },
            CreditsUser { 1975253, 7855157, "Dasshu", "Dasshu", 373, 35, 19, 12 },
            CreditsUser { 761691, 6330800, "Cvolton", "Cvolton", 101, 19, 12, -1 },
            CreditsUser { 11535118, 116938760, "alk1m123", "alk1m123", 102, 0, 3, -1 },
            CreditsUser { 5568872, 19127913, "mat4", "mat4", 453, 12, 19, 12 },
            CreditsUser { 1139015, 7834088, "Alphalaneous", "Alphalaneous", 452, 97, 42, 72 },
            CreditsUser { 7696536, 54281552, "Prevter", "Prevter", 457, 41, 63, 12 },
        }},
    };

    return g_defaultData;
}

}