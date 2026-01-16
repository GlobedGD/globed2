#include <core/hooks/LevelCell.hpp>
#include <core/net/NetworkManagerImpl.hpp>
#include <globed/config.hpp>
#include <globed/core/actions.hpp>
#include <globed/core/data/Messages.hpp>
#include <globed/core/net/MessageListener.hpp>
#include <modules/ui/UIModule.hpp>

#include <Geode/Geode.hpp>
#include <Geode/modify/LevelBrowserLayer.hpp>
#include <UIBuilder.hpp>
#include <asp/iter.hpp>

using namespace geode::prelude;

namespace globed {

static bool isValidLevelType(GJLevelType level)
{
    return (int)level == 3 || (int)level == 4;
}

struct GLOBED_MODIFY_ATTR HookedLevelBrowserLayer : geode::Modify<HookedLevelBrowserLayer, LevelBrowserLayer> {
    struct Fields {
        std::unordered_map<int, uint16_t> m_levels;
        std::optional<MessageListener<msg::PlayerCountsMessage>> m_listener;
    };

    static void onModify(auto &self)
    {
        GLOBED_CLAIM_HOOKS(UIModule::get(), self, "LevelBrowserLayer::setupLevelBrowser", );
    }

    static std::optional<GJGameLevel *> levelMapper(CCObject *obj)
    {
        auto level = typeinfo_cast<GJGameLevel *>(obj);
        return level && isValidLevelType(level->m_levelType) ? std::optional{level} : std::nullopt;
    }

    static std::optional<LevelCell *> cellMapper(CCNode *cell)
    {
        auto lc = typeinfo_cast<LevelCell *>(cell);
        return lc && isValidLevelType(lc->m_level->m_levelType) ? std::optional{lc} : std::nullopt;
    }

    $override void setupLevelBrowser(CCArray *p0)
    {
        LevelBrowserLayer::setupLevelBrowser(p0);

        auto &nm = NetworkManagerImpl::get();

        if (!p0 || !nm.isConnected()) {
            return;
        }

        auto listLayer = typeinfo_cast<LevelListLayer *>(this);

        std::vector<uint64_t> ids;

        if (listLayer) {
#ifdef GEODE_IS_ANDROID
            auto &gdLevels = listLayer->m_levelList->m_levels;
            std::vector<int> vec{gdLevels.begin(), gdLevels.end()};
            ids = asp::iter::from(vec)
#else
            ids = asp::iter::from(listLayer->m_levelList->m_levels)
#endif
                      .mapCast<uint64_t>()
                      .collect();
        } else {
            ids = asp::iter::from(CCArrayExt<CCObject *>(p0))
                      .filterMap(levelMapper)
                      .map([](GJGameLevel *level) { return (uint64_t)level->m_levelID; })
                      .collect();
        }

        m_fields->m_listener = nm.listen<msg::PlayerCountsMessage>([this](auto packet) {
            m_fields->m_levels.clear();

            for (const auto &[id, count] : packet.counts) {
                m_fields->m_levels[id.levelId()] = count;
            }

            this->refreshPagePlayerCounts();
            return ListenerResult::Continue;
        });

        nm.sendRequestPlayerCounts(std::span{ids.data(), ids.size()});
        this->schedule(schedule_selector(HookedLevelBrowserLayer::updatePlayerCounts), 5.f);
    }

    void refreshPagePlayerCounts()
    {
        if (!m_list->m_listView)
            return;

        bool inLists = typeinfo_cast<LevelListLayer *>(this);

        auto cells = m_list->m_listView->m_tableView->m_contentLayer->getChildrenExt();
        asp::iter::from(cells).filterMap(cellMapper).mapCast<HookedLevelCell *>().forEach([&](HookedLevelCell *cell) {
            int id = cell->m_level->m_levelID;
            auto it = m_fields->m_levels.find(id);

            if (it != m_fields->m_levels.end()) {
                cell->updatePlayerCount(it->second, inLists);
            } else {
                cell->updatePlayerCount(0, inLists);
            }
        });
    }

    void updatePlayerCounts(float)
    {
        auto &nm = NetworkManagerImpl::get();

        if (!nm.isConnected() || !m_list->m_listView) {
            return;
        }

        auto cells = m_list->m_listView->m_tableView->m_contentLayer->getChildrenExt();
        auto levelIds = asp::iter::from(cells)
                            .filterMap(cellMapper)
                            .map([](LevelCell *cell) { return (uint64_t)cell->m_level->m_levelID; })
                            .collect();

        nm.sendRequestPlayerCounts(std::span{levelIds.data(), levelIds.size()});
    }
};

} // namespace globed
