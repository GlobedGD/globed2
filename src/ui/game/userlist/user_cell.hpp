#pragma once
#include <defs/all.hpp>

#include <data/types/gd.hpp>
#include <game/player_store.hpp>
#include <ui/general/audio_visualizer.hpp>

class GlobedUserListPopup;

class GlobedUserCell : public cocos2d::CCLayer {
public:
    static constexpr float CELL_HEIGHT = 25.f;

    void refreshData(const PlayerStore::Entry& entry);
    void updateVisualizer(float dt);

    static GlobedUserCell* create(const PlayerStore::Entry& entry, const PlayerAccountData& data, GlobedUserListPopup* parent);

    PlayerAccountData accountData;
    bool isFriend;

private:
    Ref<cocos2d::CCLabelBMFont> percentageLabel;
    Ref<cocos2d::CCMenu> menu, usernameLayout;
    Ref<CCMenuItemSpriteExtra>
        actionsButton,
        muteButton,
        hideButton,
        kickButton,
        teleportButton,
        linkButton;

    Ref<cocos2d::CCMenu> buttonsWrapper = nullptr;
    Ref<GlobedAudioVisualizer> audioVisualizer = nullptr;
    PlayerStore::Entry _data;
    Ref<CCMenuItemSpriteExtra> nameBtn = nullptr;
    GlobedUserListPopup* parent;

    bool init(const PlayerStore::Entry& entry, const PlayerAccountData& data, GlobedUserListPopup* parent);
    void makeButtons();
    void updateUsernameLayout();
    void fixNamePosition();
};