#include <Geode/Geode.hpp>
#include <Geode/modify/LoadingLayer.hpp>
#include <globed/config.hpp>
#include <core/CoreImpl.hpp>
#include <core/PreloadManager.hpp>
#include <asp/time/SystemTime.hpp>

using namespace geode::prelude;
using namespace asp::time;

namespace globed {

struct GLOBED_MODIFY_ATTR HookedLoadingLayer : Modify<HookedLoadingLayer, LoadingLayer> {
    struct Fields {
        bool m_finished = false;
        SystemTime m_startedAt;
    };

    static void onModify(auto& self) {
        (void) self.setHookPriority("LoadingLayer::loadAssets", -100).unwrap();
    }

    // Hook loadAssets to intercept the final load step (14), to allow us to preload icons and load modules
    $override
    void loadAssets() {
        if (m_loadStep != 14 || m_fields->m_finished) {
            LoadingLayer::loadAssets();
            return;
        }

        m_fields->m_startedAt = SystemTime::now();
        PreloadManager::get().enterContext(
            m_fromRefresh ? PreloadContext::Reloading : PreloadContext::Loading
        );
        this->setLabelText("Globed: preloading assets..");
        this->schedule(schedule_selector(HookedLoadingLayer::advanceLoad), 0.0f);
    }

    void finishLoading() {
        PreloadManager::get().exitContext();
        m_fields->m_finished = true;
        log::info("Asset preloading finished in {}", m_fields->m_startedAt.elapsed().toString());

        if (!m_fromRefresh) {
            log::debug("Loading modules..");
            CoreImpl::get().onLaunch();
        }

        LoadingLayer::loadAssets();
    }

    void advanceLoad(float) {
        auto& pm = PreloadManager::get();

        if (!pm.shouldPreload()) {
            this->unschedule(schedule_selector(HookedLoadingLayer::advanceLoad));
            this->finishLoading();
            return;
        }

        pm.loadNextBatch();

        this->setLabelText(fmt::format(
            "Globed: preloading assets.. ({}/{})",
            pm.getLoadedCount(),
            pm.getTotalCount()
        ).c_str());
    }

    void setLabelText(const char* text) {
        auto label = static_cast<CCLabelBMFont*>(this->getChildByID("geode-small-label"));

        if (label) {
            label->setString(text);
        }
    }
};

}
