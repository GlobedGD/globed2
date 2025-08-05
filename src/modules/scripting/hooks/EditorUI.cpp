#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <globed/config.hpp>
#include <globed/util/scary.hpp>
#include <modules/scripting/ScriptingModule.hpp>
#include <modules/scripting/objects/FireServerObject.hpp>
#include <modules/scripting/objects/Ids.hpp>
#include <modules/scripting/ui/SetupFireServerPopup.hpp>

#include <alphalaneous.editortab_api/include/EditorTabs.hpp>

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
        auto add = [&](ScriptObjectType type) {
            if (auto btn = createObjButton(type)) {
                btn->setTag(static_cast<int>(type));
                arr->addObject(btn);
            }
        };

        add(ScriptObjectType::FireServer);

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
        auto [iobj, type] = classifyObject(m_selectedObject);
        if (type == ScriptObjectType::None) {
            return EditorUI::editObject(sender);
        }

        this->customEditObject(iobj, type);
    }

    void customEditObject(ItemTriggerGameObject* obj, ScriptObjectType type) {
        switch (type) {
            case ScriptObjectType::FireServer: {
                SetupFireServerPopup::create(static_cast<FireServerObject*>(obj))->show();
            } break;

            default: {
                log::warn("Cannot edit unknown script object type: {}", (int)type);
            } break;
        }
    }
};

CCMenuItemToggler* createObjButton(ScriptObjectType type) {
    auto tex = textureForScriptObject(type);

    auto onSprite = CCSprite::create(tex);
    auto offSprite = CCSprite::create(tex);
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
