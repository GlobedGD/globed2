#pragma once

#include <ui/BasePopup.hpp>
#include <cue/ListNode.hpp>
#include <vector>

namespace globed {

class EmoteListPopup : public BasePopup<EmoteListPopup> {
public:
    static const cocos2d::CCSize POPUP_SIZE;

    uint32_t m_selectedEmoteId = -1;
    bool m_isFavoriteMode = false;

private:
    int m_selectedPage = 0;
    int m_maxPages;
    uint32_t m_selectingFavoriteSlot = -1;

    std::vector<uint32_t> m_favoriteEmoteIds;

    cocos2d::CCMenu* m_emoteMenu;
    cocos2d::CCMenu* m_favoriteEmotesMenu;

    CCMenuItemSpriteExtra* m_clearFavoriteBtn;

    cue::ListNode* m_list;
    cue::ListNode* m_bottomList;

    CCMenuItemSpriteExtra* m_submitBtn;
    ButtonSprite* m_submitBtnSpr;

    CCMenuItemSpriteExtra* m_leftPageBtn;
    CCMenuItemSpriteExtra* m_rightPageBtn;

    cocos2d::CCLabelBMFont* m_pageLabel;

    cocos2d::extension::CCScale9Sprite* m_favoriteHighlight;
    cocos2d::CCLabelBMFont* m_favoriteInfoLabel;

    bool setup() override;
    void onSubmitBtn();
    void onEmoteBtn(uint32_t emoteId);
    void loadEmoteListPage(int page);
    void loadFavoriteEmotesList();
    void updatePage(bool increment);
    void enterFavoriteSelectMode(uint32_t emoteSlot);
    void setFavorite(uint32_t emoteSlot, uint32_t id);
};

}