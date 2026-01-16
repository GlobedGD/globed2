#pragma once

#include <globed/core/data/Messages.hpp>
#include <globed/core/net/MessageListener.hpp>
#include <globed/prelude.hpp>
#include <ui/misc/GradientLabel.hpp>

#include <cue/LoadingCircle.hpp>

namespace globed {

class FeaturedLevelCell : public CCLayer {
public:
    static FeaturedLevelCell *create();
    ~FeaturedLevelCell();

    void reloadFull();
    void reload(bool fromFullReload = false);

protected:
    static constexpr float CELL_WIDTH = 380.f;
    static constexpr float CELL_HEIGHT = 116.f;

    bool init() override;
    void updatePlayerCount(uint16_t count);
    void createCell();

    void showLoading();
    void hideLoading();
    void removeLoadedElements();

    void levelLoaded(Result<GJGameLevel *, int> result);

    void requestPlayerCount(float);

    Ref<CCMenu> m_menu;
    Ref<CCScale9Sprite> m_bg;
    Ref<CCScale9Sprite> m_loadedBg;
    Ref<CCNode> m_loadedContainer;
    Ref<CCScale9Sprite> m_playersBg;
    Ref<CCNode> m_playerCountContainer;
    Ref<CCNode> m_playerCountIcon;
    Ref<cue::LoadingCircle> m_loadingCircle;
    Ref<GradientLabel> m_playerCountLabel;

    std::optional<MessageListener<msg::PlayerCountsMessage>> m_playerCountListener;
    std::optional<MessageListener<msg::FeaturedLevelMessage>> m_levelListener;

    std::optional<FeaturedLevelMeta> m_levelMeta;
    Ref<GJGameLevel> m_level;
};

} // namespace globed
