#include "RegionSelectPopup.hpp"
#include <globed/core/ValueManager.hpp>
#include <globed/core/PopupManager.hpp>
#include <core/net/NetworkManagerImpl.hpp>

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

const CCSize RegionSelectPopup::POPUP_SIZE { 370.f, 240.f };
static constexpr CCSize LIST_SIZE { 330.f, 180.f };

namespace {
constexpr float CELL_HEIGHT = 40.f;

static ccColor3B latencyToColor(uint64_t ms) {
    std::array breakpoints = {
        std::make_pair(0, ccColor3B{0, 255, 0}),
        std::make_pair(50, ccColor3B{144, 238, 144}),
        std::make_pair(125, ccColor3B{255, 255, 0}),
        std::make_pair(200, ccColor3B{255, 165, 0}),
        std::make_pair(300, ccColor3B{255, 0, 0}),
    };

    ms = std::clamp<uint64_t>(ms, breakpoints.front().first, breakpoints.back().first);

    for (size_t i = 0; i < breakpoints.size() - 1; i++) {
        auto& [lo_val, lo_color] = breakpoints[i];
        auto& [hi_val, hi_color] = breakpoints[i + 1];

        if (ms > hi_val) {
            continue;
        }

        float t = (float)(ms - lo_val) / (float)(hi_val - lo_val);
        uint8_t r = std::round(std::lerp((float)lo_color.r, (float)hi_color.r, t));
        uint8_t g = std::round(std::lerp((float)lo_color.g, (float)hi_color.g, t));
        uint8_t b = std::round(std::lerp((float)lo_color.b, (float)hi_color.b, t));

        return ccColor3B{r, g, b};
    }

    // should never reach here
    std::unreachable();
}

class ListCell : public CCNode {
public:
    uint8_t m_serverId;

    static ListCell* create(const GameServer& server, RegionSelectPopup* popup) {
        auto ret = new ListCell;
        ret->m_popup = popup;
        ret->autorelease();
        ret->init(server);
        return ret;
    }

    void softRefreshFrom(const GameServer& server) {
        m_nameLabel->setString(server.name.c_str());
        m_regionLabel->setString(fmt::format("Region: {}", server.region).c_str());
        m_pingLabel->setPosition(m_nameLabel->getPosition() + m_nameLabel->getScaledContentSize() + CCPoint{2.f, -27.f});

        if (server.avgLatency == -1) {
            m_pingLabel->setString("? ms");
            m_pingLabel->setColor(ccColor3B{150, 150, 150});
        } else {
            m_pingLabel->setString(fmt::format("{} ms", server.avgLatency).c_str());
            m_pingLabel->setColor(latencyToColor(server.avgLatency));
        }
    }

private:
    RegionSelectPopup* m_popup;
    std::string m_stringId;
    CCLabelBMFont* m_nameLabel;
    CCLabelBMFont* m_regionLabel;
    CCLabelBMFont* m_pingLabel;
    CCNode* m_button = nullptr;
    CCMenu* m_menu;

    void init(const GameServer& server) {
        m_stringId = server.stringId;
        m_serverId = server.id;

        this->setContentSize({LIST_SIZE.width, CELL_HEIGHT});

        m_nameLabel = Build<CCLabelBMFont>::create("", "bigFont.fnt")
            .limitLabelWidth(LIST_SIZE.width * 0.75f, 0.65f, 0.1f)
            .anchorPoint(0.f, 0.5f)
            .pos(8.f, CELL_HEIGHT / 2.f + 4.f)
            .parent(this);

        m_regionLabel = Build<CCLabelBMFont>::create("", "bigFont.fnt")
            .limitLabelWidth(LIST_SIZE.width * 0.5f, 0.3f, 0.1f)
            .anchorPoint(0.f, 0.5f)
            .pos(10.f, CELL_HEIGHT / 2.f - 10.f)
            .parent(this);

        m_pingLabel = Build<CCLabelBMFont>::create("", "bigFont.fnt")
            .scale(0.35f)
            .anchorPoint(0.f, 0.f)
            .parent(this);

        m_menu = Build<CCMenu>::create()
            .layout(RowLayout::create()->setAxisAlignment(AxisAlignment::End)->setAutoScale(false))
            .contentSize(LIST_SIZE.width * 0.2f, CELL_HEIGHT * 0.7f)
            .anchorPoint(1.f, 0.5f)
            .pos(LIST_SIZE.width - 8.f, CELL_HEIGHT / 2.f)
            .parent(this)
            .collect();

        bool isPreferred = globed::value<std::string>("core.net.preferred-server") == m_stringId;

        m_button = Build<CCSprite>::createSpriteName(isPreferred ? "GJ_selectSongOnBtn_001.png" : "GJ_playBtn2_001.png")
            .with([&](auto spr) { cue::rescaleToMatch(spr, CELL_HEIGHT * 0.7f); })
            .intoMenuItem([this, isPreferred, id = m_stringId] {
                if (isPreferred) {
                    globed::setValue("core.net.preferred-server", "");
                } else {
                    globed::setValue("core.net.preferred-server", id);
                }

                m_popup->reloadList();
            })
            .parent(m_menu)
            .id("switch-btn")
            .collect();

        m_menu->updateLayout();

        this->softRefreshFrom(server);
    }
};

}

bool RegionSelectPopup::setup() {
    this->setTitle("Select Preferred Server");

    // TODO: info button in top left

    m_list = Build(cue::ListNode::create(LIST_SIZE, cue::Brown, cue::ListBorderStyle::Comments))
        .pos(this->fromCenter(0.f, -10.f))
        .parent(m_mainLayer);
    m_list->setCellHeight(CELL_HEIGHT);

    if (!this->reloadList()) {
        globed::alert("Error", "No <cy>game servers</c> are currently online.");
        return false;
    }

    Build<CCSprite>::createSpriteName("GJ_infoIcon_001.png")
        .scale(0.75f)
        .intoMenuItem([this] {
            this->showInfo();
        })
        .pos(this->fromTopRight(16.f, 16.f))
        .scaleMult(1.2f)
        .parent(m_buttonMenu);

    this->schedule(schedule_selector(RegionSelectPopup::softRefresh), 1.0f);

    return true;
}

bool RegionSelectPopup::reloadList() {
    m_list->clear();

    auto servers = NetworkManagerImpl::get().getGameServers();

    for (auto& server : servers) {
        m_list->addCell(ListCell::create(server, this));
    }

    return m_list->size() > 0;
}

void RegionSelectPopup::softRefresh(float) {
    auto servers = NetworkManagerImpl::get().getGameServers();

    std::vector<size_t> toRemove;

    size_t i = 0;
    for (auto cell : m_list->iter<ListCell>()) {
        bool refreshed = false;

        for (auto& srv : servers) {
            if (srv.id == cell->m_serverId) {
                cell->softRefreshFrom(srv);
                refreshed = true;
                break;
            }
        }

        if (!refreshed) {
            log::warn("Removing server with ID {}, no longer exists", cell->m_serverId);
            toRemove.push_back(i);
        }

        i++;
    }

    for (auto it = toRemove.rbegin(); it != toRemove.rend(); it++) {
        m_list->removeCell(*it);
    }

    // force refresh if new servers may have appeared
    if (servers.size() != m_list->size()) {
        this->reloadList();
    }
}

void RegionSelectPopup::showInfo() {
    globed::alert(
        "Note",
        "Multiple <cj>game servers</c> can be located in different regions, to ensure everyone in the world can have a great experience. "
        "By default, Globed will try to use the server with the <cg>lowest ping</c>, but you can manually choose your <cy>preferred server</c> if you prefer playing with different people.\n\n"
        "Note: you will still be able to join people in <cy>other servers</c>. This setting impacts which <cp>Global Room</c> you play in, and which server <cj>rooms created by you</c> will be in.",
        "Ok",
        nullptr,
        410.f
    );
}

}