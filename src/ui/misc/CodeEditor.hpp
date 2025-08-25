#pragma once

#include <Geode/Geode.hpp>
#include <globed/util/CStr.hpp>

#include <AdvancedLabel.hpp>

namespace globed {

class CodeEditor : public cocos2d::CCLayer {
public:
    static CodeEditor* create(cocos2d::CCSize size);

    void setContent(CStr content);
    void setFontSize(float scale);

private:
    cocos2d::CCSize m_size;
    cocos2d::CCLayerColor* m_background;
    geode::ScrollLayer* m_scrollLayer;
    CCNode* m_textContainer;
    std::vector<Label*> m_textLabels;
    CCNode* m_lineNumContainer;
    std::vector<Label*> m_lineNumLabels;
    std::string m_textBuffer;
    size_t m_cursorPos = -1;
    cocos2d::CCPoint m_cursorUiPos{};
    cocos2d::CCLayerColor* m_cursor = nullptr;
    float m_textScale = 0.5f;
    bool m_activeTouch = false;

    bool init(cocos2d::CCSize size);

    void updateFromBuffer();
    void setCursorPos(size_t pos);
    void setCursorUiPos(cocos2d::CCPoint);

    void updateState(bool recreate = false);
    void splitStringInto(std::string_view str, std::vector<Label*>& labels, CCNode* container, BMFontAlignment alignment, uint8_t opacity);

    // Touch stuff

    bool ccTouchBegan(cocos2d::CCTouch *pTouch, cocos2d::CCEvent *pEvent) override;
    void ccTouchMoved(cocos2d::CCTouch *pTouch, cocos2d::CCEvent *pEvent) override;
    void ccTouchEnded(cocos2d::CCTouch *pTouch, cocos2d::CCEvent *pEvent) override;
    void ccTouchCancelled(cocos2d::CCTouch *pTouch, cocos2d::CCEvent *pEvent) override;

    std::pair<size_t, cocos2d::CCPoint> touchPosToBufferPos(cocos2d::CCPoint pos);


};

}