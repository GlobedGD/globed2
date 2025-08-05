#include <globed/config.hpp>
#include <modules/scripting/ScriptingModule.hpp>
#include <modules/scripting/objects/Ids.hpp>
#include "Common.hpp"

#include <alphalaneous.editortab_api/include/EditorTabs.hpp>
#include <cue/Util.hpp>
#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>

using namespace geode::prelude;

namespace globed {

CCMenuItemToggler* createObjButton(ScriptObjectType type);

struct GLOBED_NOVTABLE GLOBED_DLL SCEditorUIHook : geode::Modify<SCEditorUIHook, EditorUI> {
    struct Fields {
        EditButtonBar* m_buttonBar = nullptr;
    };

    static SCEditorUIHook* get() {
        return static_cast<SCEditorUIHook*>(EditorUI::get());
    }

    static void onModify(auto& self) {
        // GLOBED_CLAIM_HOOKS(ScriptingModule::get(), self,
        //     "EditorUI::init",
        // );
    }

    $override
    bool init(LevelEditorLayer* layer) {
        if (!EditorUI::init(layer)) return false;

        auto arr = CCArray::create();

        for (auto type : ALL_SCRIPT_OBJECT_TYPES) {
            if (auto btn = createObjButton(type)) {
                btn->setTag(static_cast<int>(type));
                arr->addObject(btn);
            }
        }

        m_fields->m_buttonBar = EditorTabUtils::createEditButtonBar(arr, this);

        EditorTabs::addTab(this, TabType::BUILD, "scripting"_spr, [this](EditorUI* ui, CCMenuItemToggler* toggler) -> CCNode* {
            auto sprite = CCSprite::create("globed-gold-icon.png"_spr);
            sprite->setScale(0.2f);
            EditorTabUtils::setTabIcon(toggler, sprite);
            return m_fields->m_buttonBar;
        });

        return true;
    }

    void deselectAllCustom(CCObject* except) {
        // deselect all
        for (auto btn : CCArrayExt<CCMenuItemToggler>(m_fields->m_buttonBar->m_buttonArray)) {
            if (btn == except) continue;

            btn->toggle(false);
        }
    }

    $override
    void editObject(CCObject* sender) {
        if (!globed::onEditObject(m_selectedObject)) {
            return EditorUI::editObject(sender);
        }
    }
};

CCMenuItemToggler* createObjButton(ScriptObjectType type) {
    auto tex = textureForScriptObject(type);

    auto onSprite = ButtonSprite::create(CCSprite::create(tex), 40.f, 0, 40.f, 1.0f, false, "GJ_button_04.png", false);
    auto offSprite = ButtonSprite::create(CCSprite::create(tex), 40.f, 0, 40.f, 1.0f, false, "GJ_button_04.png", false);

    cue::rescaleToMatch(onSprite, {40.f, 40.f});
    cue::rescaleToMatch(offSprite, {40.f, 40.f});

    onSprite->setColor(ccc3(100, 100, 100));

    if (!onSprite || !offSprite) {
        log::warn("Failed to load sprite for script object type: {} ({})", (int)type, tex);
        return nullptr;
    }

    return CCMenuItemExt::createToggler(onSprite, offSprite, [](auto toggler) {
        bool on = !toggler->isSelected();
        auto type = static_cast<ScriptObjectType>(toggler->getTag());

        SCEditorUIHook::get()->deselectAllCustom(toggler);

        if (on) {
            // select
            auto ui = EditorUI::get();
            ui->m_selectedObjectIndex = SCRIPT_OBJECT_IDENT_MASK | static_cast<uint32_t>(type);
        }
    });
}

}
