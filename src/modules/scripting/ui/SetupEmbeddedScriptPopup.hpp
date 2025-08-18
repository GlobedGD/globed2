#pragma once

#include <modules/scripting/data/EmbeddedScript.hpp>
#include <ui/BasePopup.hpp>
#include <ui/misc/CodeEditor.hpp>

#include <Geode/Geode.hpp>

namespace globed {

class SetupEmbeddedScriptPopup : public BasePopup<SetupEmbeddedScriptPopup, TextGameObject*> {
    friend class BasePopup;
    static const cocos2d::CCSize POPUP_SIZE;

public:
    bool setup(TextGameObject* obj) override;

private:
    TextGameObject* m_object;
    EmbeddedScript m_script{};
    std::string m_scriptPrefix;
    bool m_doSave = false;
    bool m_confirmedExit = false;
    bool m_madeChanges = false;
    CodeEditor* m_editor;

    void onClose(CCObject*) override;
    void onUpload();
    void invalidateSignature();
    geode::Result<> loadCodeFromPath(const std::filesystem::path& path);
};

}
