#include "Common.hpp"
#include "GJBaseGameLayer.hpp"
#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <globed/config.hpp>
#include <globed/util/scary.hpp>
#include <modules/scripting/ScriptingModule.hpp>
#include <modules/scripting/objects/FireServerObject.hpp>
#include <modules/scripting/objects/Ids.hpp>

using namespace geode::prelude;

namespace globed {

struct GLOBED_MODIFY_ATTR SCPlayLayerHook : geode::Modify<SCPlayLayerHook, PlayLayer> {
    struct Fields {
        bool m_hasScriptObjects = false;
        std::vector<EmbeddedScript> m_scripts;
    };

    static void onModify(auto &self)
    {
        (void)self.setHookPriority("PlayLayer::addObject", -1000);

        GLOBED_CLAIM_HOOKS(ScriptingModule::get(), self, "PlayLayer::setupHasCompleted", "PlayLayer::addObject", );
    }

    $override void addObject(GameObject *p0)
    {
        std::optional<EmbeddedScript> script;

        if (globed::onAddObject(p0, false, script)) {
            auto &fields = *m_fields.self();
            fields.m_hasScriptObjects = true;

            if (script) {
                // add the script
                fields.m_scripts.push_back(std::move(*script));
            }
        }

        PlayLayer::addObject(p0);
    }

    $override void setupHasCompleted()
    {
        PlayLayer::setupHasCompleted();

        auto &fields = *m_fields.self();

        if (!fields.m_hasScriptObjects) {
            log::debug("No script objects, disabling module");
            (void)ScriptingModule::get().disable();
            return;
        }

        auto layer = SCBaseGameLayer::get(this);
        layer->postInit(fields.m_scripts);
    }
};

} // namespace globed
