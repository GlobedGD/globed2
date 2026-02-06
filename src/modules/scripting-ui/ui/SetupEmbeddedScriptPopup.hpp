#pragma once

#include <modules/scripting/data/EmbeddedScript.hpp>
#include <ui/BasePopup.hpp>
#include <ui/misc/CodeEditor.hpp>

#include <Geode/Geode.hpp>

namespace globed {

class SetupEmbeddedScriptPopup : public BasePopup {
public:
    static SetupEmbeddedScriptPopup* create(TextGameObject* obj);

private:
    TextGameObject* m_object;
    EmbeddedScript m_script{};
    std::string m_scriptPrefix;
    bool m_doSave = false;
    bool m_confirmedExit = false;
    bool m_madeChanges = false;
    CodeEditor* m_editor;

    bool init(TextGameObject* obj);
    void onClose(CCObject*) override;
    void onUpload();
    void onPaste();
    void invalidateSignature();
    geode::Result<> loadCodeFromPath(const std::filesystem::path& path);
    void loadCodeFromString(std::string&& code);
};

}
