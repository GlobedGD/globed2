#include "SetupEmbeddedScriptPopup.hpp"
#include <globed/core/PopupManager.hpp>

#include <UIBuilder.hpp>
#include <asp/fs.hpp>
#include <cue/Util.hpp>

using namespace geode::prelude;

namespace globed {

const CCSize SetupEmbeddedScriptPopup::POPUP_SIZE {460.f, 300.f};

bool SetupEmbeddedScriptPopup::setup(TextGameObject* obj) {
    auto& text = obj->m_text;

    m_object = obj;

    auto res = EmbeddedScript::decode(std::span{(uint8_t*) text.data(), text.size()});
    if (!res) {
        log::error("Failed to decode embedded script data: {}", res.unwrapErr());

        // fill with defaults
        m_script.content = fmt::format("-- Failed to read script data\n-- {}", res.unwrapErr());
        m_script.filename = "script.lua";
        m_script.main = false;
        m_scriptPrefix = "GLOBED_SCRIPT";

        return true;
    }

    m_script = *res;
    for (char c : text) {
        if (c == '\0') break;
        m_scriptPrefix += c;
    }

    // setup ui itself

    this->setTitle("Edit Globed Script");
    m_title->setPositionY(m_title->getPositionY() + 6.f);

    // confirm / cancel buttons
    auto bottomMenu = Build<CCMenu>::create()
        .layout(RowLayout::create()->setGap(3.f)->setAutoScale(false))
        .contentSize(POPUP_SIZE.width, 60.f)
        .pos(this->fromBottom(27.f))
        .parent(m_mainLayer)
        .collect();

    Build<ButtonSprite>::create("Discard", "goldFont.fnt", "GJ_button_01.png", 1.0f)
        .scale(0.85f)
        .intoMenuItem([this] {
            m_doSave = false;
            this->onClose(nullptr);
        })
        .scaleMult(1.15f)
        .parent(bottomMenu);

    Build<ButtonSprite>::create("Save", "goldFont.fnt", "GJ_button_01.png", 1.0f)
        .scale(0.85f)
        .intoMenuItem([this] {
            m_doSave = true;
            this->onClose(nullptr);
        })
        .scaleMult(1.15f)
        .parent(bottomMenu);

    bottomMenu->updateLayout();

    // filename input

    auto filenameInput = Build(TextInput::create(200.f, "Script name"))
        .pos(this->fromTop(42.f))
        .parent(m_mainLayer)
        .collect();

    filenameInput->setString(m_script.filename);
    filenameInput->setCommonFilter(CommonFilter::Any);
    filenameInput->setCallback([this](auto name) {
        m_script.filename = name;
        m_madeChanges = true;
    });

    // editor
    m_editor = Build(CodeEditor::create({400.f, 180.f}))
        .pos(this->fromCenter(0.f, -10.f))
        .parent(m_mainLayer);

    m_editor->setContent(m_script.content);

    // checkbox
    auto toggler = Build(CCMenuItemExt::createTogglerWithStandardSprites(0.8f, [this](auto toggler) {
        bool on = !toggler->isOn();
        m_script.main = !m_script.main;
    }))
        .pos(24.f, 24.f)
        .parent(m_buttonMenu)
        .collect();
    toggler->m_offButton->m_scaleMultiplier = 1.1f;
    toggler->m_onButton->m_scaleMultiplier = 1.1f;
    toggler->toggle(m_script.main);

    Build<CCLabelBMFont>::create("Main", "bigFont.fnt")
        .scale(0.45f)
        .pos(60.f, 24.f)
        .parent(m_mainLayer);

    CCSize btnSize {32.f, 32.f};

    // upload btn
    Build<CCSprite>::createSpriteName("GJ_myServerBtn_001.png")
        .with([&](auto spr) { cue::rescaleToMatch(spr, btnSize); })
        .intoMenuItem([this] {
            this->onUpload();
        })
        .pos(this->fromBottomRight(24.f, 26.f))
        .scaleMult(1.15f)
        .parent(m_buttonMenu);

    // paste btn
    Build(CircleButtonSprite::createWithSprite("clipboard-icon.png"_spr, 1.f, CircleBaseColor::Pink, CircleBaseSize::Small))
        .with([&](auto spr) { cue::rescaleToMatch(spr, btnSize); })
        .intoMenuItem([this] {
            this->onPaste();
        })
        .pos(this->fromBottomRight(60.f, 26.f))
        .scaleMult(1.15f)
        .parent(m_buttonMenu);

    return true;
}

void SetupEmbeddedScriptPopup::onUpload() {
    auto task = geode::utils::file::pick(
        file::PickMode::OpenFile,
        file::FilePickOptions {
            .filters = {{
                .description = "Lua Script Files (*.lua)",
                .files = {"*.lua"}
            }}
        }
    );

    task.listen([this](Result<std::filesystem::path>* result) {
        if (!result || !*result) {
            log::info("Failed to upload file: {}", result->unwrapErr());
            return;
        }

        auto res = this->loadCodeFromPath(**result);

        if (!res) {
            globed::alertFormat("Error", "Failed to import the given file: {}", res.unwrapErr());
        }
    });
}

void SetupEmbeddedScriptPopup::onPaste() {
    this->loadCodeFromString(geode::utils::clipboard::read());
}

Result<> SetupEmbeddedScriptPopup::loadCodeFromPath(const std::filesystem::path& path) {
    auto code = GEODE_UNWRAP(asp::fs::readToString(path));
    this->loadCodeFromString(std::move(code));

    return Ok();
}

void SetupEmbeddedScriptPopup::loadCodeFromString(std::string&& code) {
    m_editor->setContent(code);
    m_script.content = std::move(code);
    m_madeChanges = true;
    this->invalidateSignature();
}

void SetupEmbeddedScriptPopup::invalidateSignature() {
    m_script.signature.reset();
    // TODO (scr): some ui way to show
}

void SetupEmbeddedScriptPopup::onClose(CCObject* obj) {
    if (m_madeChanges && !m_doSave && !m_confirmedExit) {
        globed::confirmPopup(
            "Note",
            "Are you sure you want to <cr>discard</c> all the changes you've made?",
            "Cancel", "Discard",
            [this](auto) {
                m_confirmedExit = true;
                this->onClose(nullptr);
            }
        );

        return;
    }

    // commit changes
    if (m_doSave) {
        auto res = m_script.encode(true, m_scriptPrefix);
        if (!res) {
            log::error("Failed to save script: {}", res.unwrapErr());
            globed::alertFormat("Error", "Failed to save script: {}", res.unwrapErr());
        } else {
            auto data = std::move(res).unwrap();
            m_object->m_text = gd::string((const char*) data.data(), data.size());
        }
    }

    BasePopup::onClose(obj);
}

}
