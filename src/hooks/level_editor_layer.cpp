#include "level_editor_layer.hpp"

#include <Geode/utils/web.hpp>

#include "gjbasegamelayer.hpp"
#include "triggers/gjeffectmanager.hpp"
#include <managers/settings.hpp>
#include <managers/hook.hpp>
#include <managers/popup_queue.hpp>

using namespace geode::prelude;

/* Hooks */

bool GlobedLevelEditorLayer::init(GJGameLevel* level, bool p1) {
    GlobedLevelEditorLayer::fromEditor = true;

    auto gjbgl = static_cast<GlobedGJBGL*>(static_cast<GJBaseGameLayer*>(this));

    gjbgl->setupPreInit(level, true);

    if (!LevelEditorLayer::init(level, p1)) return false;

    gjbgl->setupAll();

    gjbgl->m_fields->setupWasCompleted = true;

    auto& settings = GlobedSettings::get();
    if (settings.globed.editorChanges) {
        HookManager::get().enableGroup(HookManager::Group::EditorTriggerPopups);

        if (!settings.flags.seenGlobalTriggerGuide) {
            settings.flags.seenGlobalTriggerGuide = true;
            auto alert = geode::createQuickPopup("Globed", fmt::format("Visit the global trigger guide page? <cy>({})</c>\n\n<cg>Highly recommended</c>, if you've never worked with them before.", globed::string<"global-trigger-page">()), "No", "Yes", [](auto, bool agree) {
                if (!agree) return;

                geode::utils::web::openLinkInBrowser(globed::string<"global-trigger-page">());
            }, false);

            PopupQueue::get()->push(alert);
        }
    } else {
        HookManager::get().disableGroup(HookManager::Group::EditorTriggerPopups);
    }

    globed::toggleEditorTriggerHooks(settings.globed.editorChanges);

    return true;
}
