#pragma once

#include <Geode/Geode.hpp>
#include <globed/util/CStr.hpp>

namespace globed {

class CodeEditor : public cocos2d::CCLayer {
public:
    static CodeEditor* create(cocos2d::CCSize size);

    void setContent(CStr content);
    void setFontSize(float scale);

private:
    cocos2d::CCSize m_size;
    cocos2d::CCLayerColor* m_background;
    cocos2d::CCLabelBMFont* m_label;
    geode::ScrollLayer* m_scrollLayer;
    cocos2d::CCLabelBMFont* m_lineNumberColumn = nullptr;
    std::string m_textBuffer;
    size_t m_cursorPos = -1;
    cocos2d::CCPoint m_cursorUiPos{};
    cocos2d::CCLayerColor* m_cursor = nullptr;
    bool m_activeTouch = false;

    bool init(cocos2d::CCSize size);
    void updateScrollLayer();
    void updateLineNumbers();
    void updateFromBuffer();

    void setCursorPos(size_t pos);
    void setCursorUiPos(cocos2d::CCPoint);


    // Touch stuff

    bool ccTouchBegan(cocos2d::CCTouch *pTouch, cocos2d::CCEvent *pEvent) override;
    void ccTouchMoved(cocos2d::CCTouch *pTouch, cocos2d::CCEvent *pEvent) override;
    void ccTouchEnded(cocos2d::CCTouch *pTouch, cocos2d::CCEvent *pEvent) override;
    void ccTouchCancelled(cocos2d::CCTouch *pTouch, cocos2d::CCEvent *pEvent) override;

    std::pair<size_t, cocos2d::CCPoint> touchPosToBufferPos(cocos2d::CCPoint pos);


};

}