#include "SendFeaturePopup.hpp"
#include <globed/util/gd.hpp>
#include <core/net/NetworkManagerImpl.hpp>
#include <ui/menu/FeatureCommon.hpp>

#include <UIBuilder.hpp>
#include <cue/Util.hpp>

using namespace geode::prelude;

namespace globed {

const CCSize SendFeaturePopup::POPUP_SIZE{ 340.f, 190.f };

bool SendFeaturePopup::setup(GJGameLevel* level) {
    m_level = level;

    this->setTitle("Globed: Send Level", "goldFont.fnt", 0.9f, 20.f);

    m_menu = Build<CCMenu>::create()
        .pos(this->fromTop(77.f))
        .parent(m_mainLayer)
        .collect();

    auto diff = globed::calcLevelDifficulty(level);

    Build<GJDifficultySprite>::create((int)diff, GJDifficultyName::Short)
        .pos(this->fromTop(77.f))
        .parent(m_mainLayer)
        .scale(1.25f)
        .zOrder(3);

    this->createDiffButton();

    m_noteInput = Build<TextInput>::create(POPUP_SIZE.width * 0.8f, "Notes", "chatFont.fnt")
        .pos(this->fromBottom(60.f))
        .parent(m_mainLayer);
    m_noteInput->setCommonFilter(CommonFilter::Any);

    auto* buttonMenu = Build<CCMenu>::create()
        .layout(RowLayout::create())
        .contentSize(POPUP_SIZE.width * 0.8f, 50.f)
        .pos(this->fromBottom(27.f))
        .parent(m_mainLayer)
        .collect();

    Build<ButtonSprite>::create("Send", "bigFont.fnt", "GJ_button_01.png", 0.9f)
        .scale(0.75f)
        .intoMenuItem([this] {
            this->sendLevel(false);
        })
        .parent(buttonMenu);

    if (NetworkManagerImpl::get().getModPermissions().canRateFeatures) {
        Build<ButtonSprite>::create("Queue", "bigFont.fnt", "GJ_button_01.png", 0.9f)
            .scale(0.75f)
            .intoMenuItem([this] {
                this->sendLevel(true);
            })
            .parent(buttonMenu);
    }

    buttonMenu->updateLayout();

    return true;
}

void SendFeaturePopup::sendLevel(bool queue) {
    auto& nm = NetworkManagerImpl::get();
    nm.sendSendFeaturedLevel(
        m_level->m_levelID,
        m_level->m_levelName,
        m_level->m_accountID,
        m_level->m_creatorName,
        (uint8_t)m_chosenTier,
        m_noteInput->getString(),
        queue
    );

    globed::toastSuccess("Successfully {} level!", queue ? "queued" : "sent");
    this->onClose(this);
}

void SendFeaturePopup::createDiffButton() {
    cue::resetNode(m_diffButton);

    Build(globed::createRatingSprite(m_chosenTier))
        .scale(1.25f)
        .intoMenuItem([this] {
            switch (m_chosenTier) {
                case FeatureTier::Normal: m_chosenTier = FeatureTier::Epic; break;
                case FeatureTier::Epic: m_chosenTier = FeatureTier::Outstanding; break;
                case FeatureTier::Outstanding:
                default:
                    m_chosenTier = FeatureTier::Normal; break;
            }

            this->createDiffButton();
        })
        .parent(m_menu);
}

}
