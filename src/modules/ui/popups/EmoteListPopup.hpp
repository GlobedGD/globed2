#pragma once

#include <globed/aliases.hpp>
#include <ui/Core.hpp>
#include <ui/BasePopup.hpp>
#include <cue/ListNode.hpp>
#include <vector>

namespace globed {

class EmoteListPopup : public BasePopup {
public:
    static EmoteListPopup* create();

    uint32_t m_selectedEmoteId = -1;
    bool m_isFavoriteMode = false;

private:
    int m_selectedPage = 0;
    int m_maxPages;
    uint32_t m_selectingFavoriteSlot = -1;

    std::vector<uint32_t> m_favoriteEmoteIds;

    cocos2d::CCMenu* m_emoteMenu;
    cocos2d::CCMenu* m_favoriteEmotesMenu;

    geode::Button* m_clearFavoriteBtn;

    cue::ListNode* m_list;
    cue::ListNode* m_bottomList;

    geode::Button* m_submitBtn;
    ButtonSprite* m_submitBtnSpr;

    geode::Button* m_leftPageBtn;
    geode::Button* m_rightPageBtn;

    Label* m_pageLabel;

    cocos2d::extension::CCScale9Sprite* m_favoriteHighlight;
    Label* m_favoriteInfoLabel;

    cue::Slider* m_volumeSlider;

    bool init() override;
    void onSubmitBtn();
    void onEmoteBtn(uint32_t emoteId);
    void loadEmoteListPage(int page);
    void loadFavoriteEmotesList();
    void updatePage(bool increment);
    void enterFavoriteSelectMode(uint32_t emoteSlot);
    void setFavorite(uint32_t emoteSlot, uint32_t id);
};

}