#include <Geode/Geode.hpp>
#include <Geode/modify/LoadingLayer.hpp>
#include <globed/config.hpp>
#include <core/Core.hpp>
#include <core/preload/PreloadManager.hpp>
#include <asp/time/SystemTime.hpp>
#include <asp/fs.hpp>

using namespace geode::prelude;
using namespace asp::time;

namespace globed {

struct GLOBED_MODIFY_ATTR HookedLoadingLayer : Modify<HookedLoadingLayer, LoadingLayer> {
    struct Fields {
        bool m_finished = false;
        SystemTime m_startedAt;
        AsyncLoad::TaskGroup m_group;
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
        m_fields->m_group = PreloadManager::get().enterContext(
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
            Core::get().onLaunch();
        }

        // load custom globed textures
        if (auto e = this->loadCustomTextures().err()) {
            log::warn("loadCustomTextures failed: {}", *e);
        }

        LoadingLayer::loadAssets();
    }

    void advanceLoad(float) {
        auto& pm = PreloadManager::get();
        auto& group = m_fields->m_group;

        size_t total = group.totalTasks();
        size_t pending = group.pendingTasks();
        size_t loaded = total - pending;

        if (pending == 0) {
            this->unschedule(schedule_selector(HookedLoadingLayer::advanceLoad));
            this->finishLoading();
            return;
        }

        auto updateLabel = [this](size_t loaded, size_t total) {
            StringBuffer<> buf;

            if (loaded < total) {
                buf.append("Globed: preloading assets.. ({}/{})", loaded, total);
            } else {
                buf.append("Globed: finalizing..");
            }

            this->setLabelText(buf.c_str());
        };

        updateLabel(loaded, total);

        AsyncLoad::ALManager::get().lendMainThread();
    }

    void setLabelText(const char* text) {
        auto label = typeinfo_cast<Label*>(this->getChildByID("geode-small-label"));

        if (label) {
            label->setString(text);
        }
    }

    Result<> loadCustomTextures() {
        // these are some textures that are likely to be used by globed,
        // we can preload them to avoid a small lagspike when the texture has to be loaded via CCSprite::create

        auto entries = std::to_array<ZStringView>({
            "deafen-icon-off.png"_spr,
            "deafen-icon-on.png"_spr,
            "discord01.png"_spr,
            "exit01.png"_spr,
            "exit02.png"_spr,
            "featured-player-icon.png"_spr,
            "friend-gradient.png"_spr,
            "globed-gold-icon.png"_spr,
            "icon-emotes.png"_spr,
            "icon-friend.png"_spr,
            "icon-gear.png"_spr,
            "icon-glow.png"_spr,
            "icon-invite.png"_spr,
            "icon-leave-server.png"_spr,
            "icon-mute.png"_spr,
            "icon-person.png"_spr,
            "icon-players.png"_spr,
            "icon-refresh-square.png"_spr,
            "icon-show-player.png"_spr,
            "icon-unmute.png"_spr,
            "invite.png"_spr,
            "levels01.png"_spr,
            "list01.png"_spr,
            "privacySettings.png"_spr,
            "refresh01.png"_spr,
            "role-admin.png"_spr,
            "role-booster.png"_spr,
            "role-cc1-winner.png"_spr,
            "role-comm-figure.png"_spr,
            "role-contrib.png"_spr,
            "role-dev.png"_spr,
            "role-helper.png"_spr,
            "role-mod.png"_spr,
            "role-owner.png"_spr,
            "role-server-owner.png"_spr,
            "role-supporter.png"_spr,
            "role-trial-mod.png"_spr,
            "role-unknown.png"_spr,
            "role-user.png"_spr,
            "search01.png"_spr,
            "search02.png"_spr,
            "server01.png"_spr,
            "server02.png"_spr,
            "settings01.png"_spr,
            "slider-end.png"_spr,
            "slider-fill.png"_spr,
            "slider-middle.png"_spr,
            "slider-start.png"_spr,
            "slider-thumb.png"_spr,
            "slider-thumb-sel.png"_spr,
            "speaker-icon.png"_spr,
            "speaker-icon-mute.png"_spr,
            "support01.png"_spr,
            "support02.png"_spr,
            "teams01.png"_spr,
            "teams02.png"_spr,
        });

        for (auto entry : entries) {
            GEODE_UNWRAP(AsyncLoad::preload(entry));
        }

        return Ok();
    }
};

}
