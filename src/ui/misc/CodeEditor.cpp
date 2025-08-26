#include "CodeEditor.hpp"
#include <globed/util/color.hpp>

#include <UIBuilder.hpp>

using namespace geode::prelude;

constexpr static float TEXT_PAD = 5.f;
constexpr static float LINE_NUMBER_WIDTH = 30.f;

namespace globed {

static CCSize getEstimateLetterSize(float scale) {
    auto label = CCLabelBMFont::create("Among us? Impostor sus sus sus", "Consolas.fnt"_spr);
    label->setScale(scale);
    auto size = label->getScaledContentSize();

    return {size.width / strlen(label->getString()), size.height};
}

bool CodeEditor::init(CCSize size) {
    if (!CCLayer::init()) return false;

    m_size = size;

    this->setTouchEnabled(true);
    this->setTouchMode(cocos2d::kCCTouchesOneByOne);
    this->setContentSize(size);
    this->ignoreAnchorPointForPosition(false);
    this->setAnchorPoint({0.5f, 0.5f});

    m_background = Build(CCLayerColor::create(colorFromHex("#1e1e1e"), size.width, size.height))
        .zOrder(-4)
        .anchorPoint(0.5f, 0.5f)
        .ignoreAnchorPointForPos(false)
        .pos(size / 2.f)
        .parent(this);

    auto scrollSize = size - CCSize{TEXT_PAD * 2.f, TEXT_PAD * 2.f};

    m_scrollLayer = Build(ScrollLayer::create(scrollSize))
        .anchorPoint(0.5f, 0.5f)
        .ignoreAnchorPointForPos(false)
        .pos(size / 2.f)
        .parent(this);

    m_textContainer = Build<CCNode>::create()
        .id("text-container")
        .layout(ColumnLayout::create()->setAxisReverse(true)->setAutoScale(false)->setGap(0.f))
        .contentSize(scrollSize.width - LINE_NUMBER_WIDTH, 0.f)
        .anchorPoint(0.f, 1.f)
        .pos(LINE_NUMBER_WIDTH, scrollSize.height)
        .parent(m_scrollLayer->m_contentLayer);

    m_lineNumContainer = Build<CCNode>::create()
        .id("line-num-container")
        .layout(ColumnLayout::create()->setAxisReverse(true)->setAutoScale(false)->setGap(0.f))
        .contentSize(LINE_NUMBER_WIDTH, 0.f)
        .anchorPoint(0.f, 1.f)
        .pos(4.f, scrollSize.height)
        .parent(m_scrollLayer->m_contentLayer);

    this->setFontSize(0.5f);

    return true;
}

void CodeEditor::setContent(CStr content) {
    m_textBuffer.clear();

    for (char c : std::string_view{content}) {
        if (c == '\t') {
            for (size_t i = 0; i < 4; i++) m_textBuffer.push_back(' ');
        } else {
            m_textBuffer.push_back(c);
        }
    }

    this->updateFromBuffer();
}

void CodeEditor::updateFromBuffer() {
    this->updateState(false);
}

void CodeEditor::setFontSize(float scale) {
    m_textScale = scale;

    if (m_cursor) m_cursor->removeFromParent();

    auto lsize = getEstimateLetterSize(scale);
    m_cursor = Build(CCLayerColor::create({255, 255, 255, 255}, 1.f, lsize.height + 1.f))
        .opacity(0)
        .ignoreAnchorPointForPos(false)
        .anchorPoint(0.5f, 0.5f)
        .pos(m_cursorUiPos)
        .parent(m_scrollLayer->m_contentLayer);

    this->updateState(true);
}

void CodeEditor::splitStringInto(std::string_view str, std::vector<Label*>& labels, CCNode* container, BMFontAlignment alignment, uint8_t opacity) {
    size_t curIdx = 0;

    auto addOne = [&](std::string_view s) {
        if (curIdx >= labels.size()) {
            auto label = Label::create("", "Consolas.fnt"_spr, alignment, m_textScale);
            label->setOpacity(opacity);
            labels.push_back(label);
            container->addChild(label);
        }

        auto label = labels[curIdx++];
        label->setString(s);
    };

    size_t contentBegin = 0;
    size_t inLine = 0;

    for (char c : str) {
        // allow up to 10000 characters in one label, and then scan until a newline
        if (inLine > 10000 && c == '\n') {
            addOne(str.substr(contentBegin, inLine));
            contentBegin += inLine + 1;
            inLine = 0;
        } else {
            inLine++;
        }
    }

    if (inLine > 0) {
        addOne(str.substr(contentBegin, inLine));
    }

    double totalHeight = 0.f;
    for (auto label : labels) {
        totalHeight += label->getContentHeight();
    }

    container->setContentHeight(totalHeight * (double)m_textScale);
    container->updateLayout();

}

void CodeEditor::updateState(bool recreate) {
    if (recreate) {
        m_textContainer->removeAllChildren();
        m_lineNumContainer->removeAllChildren();
        m_textLabels.clear();
        m_lineNumLabels.clear();
    }

    // calculate amount of lines and build the line string

    size_t lines = 1;

    for (char c : m_textBuffer) {
        if (c == '\n') lines++;
    }

    std::string outStr;
    for (size_t i = 0; i < lines; i++) {
        std::array<char, 32> buf;
        auto result = fmt::format_to_n(buf.data(), buf.size(), "{}\n", i + 1);

        for (size_t j = 0; j < result.size; j++) {
            if (i == lines - 1 && j == result.size - 1) {
                // dont insert the final newline
                continue;
            }

            outStr.push_back(buf[j]);
        }

        // if (i == 0) {
        //     // bruh
        //     for (size_t j = result.size; j < 4; j++) {
        //         outStr.push_back(' ');
        //     }
        // }
    }

    splitStringInto(outStr, m_lineNumLabels, m_lineNumContainer, BMFontAlignment::Right, 160);
    splitStringInto(m_textBuffer, m_textLabels, m_textContainer, BMFontAlignment::Left, 255);

    auto csize = m_textContainer->getScaledContentSize();
    if (csize.height < m_scrollLayer->getContentHeight()) {
        csize.height = m_scrollLayer->getContentHeight();
    }

    m_scrollLayer->m_contentLayer->setContentSize(csize);

    for (auto lbl : m_lineNumLabels) {
        lbl->setContentWidth(LINE_NUMBER_WIDTH);
    }

    m_lineNumContainer->updateLayout();

    float yPos = csize.height;
    m_lineNumContainer->setPositionY(yPos);
    m_textContainer->setPositionY(yPos);
}

void CodeEditor::setCursorPos(size_t pos) {
    m_cursorPos = pos;
}

void CodeEditor::setCursorUiPos(CCPoint pos) {
    return; // TODO
    m_cursorUiPos = pos;
    m_cursor->setPosition(pos);
    m_cursor->setVisible(true);
}

bool CodeEditor::ccTouchBegan(cocos2d::CCTouch *pTouch, cocos2d::CCEvent *pEvent) {
    // TODO, i dont want to continue struggling here
    return false;

    auto pos = this->convertTouchToNodeSpace(pTouch);
    if (!CCRect{{0, 0}, m_size}.containsPoint(pos)) {
        return false;
    }

    auto [bpos, curpos] = this->touchPosToBufferPos(pos);

    if (bpos == -1) {
        // no code was touched
        return true;
    }

    this->setCursorPos(bpos);
    this->setCursorUiPos(curpos);
    m_activeTouch = true;

    return true;
}

void CodeEditor::ccTouchMoved(CCTouch *pTouch, CCEvent *pEvent) {

}

void CodeEditor::ccTouchEnded(CCTouch *pTouch, CCEvent *pEvent) {
    m_activeTouch = false;
}

void CodeEditor::ccTouchCancelled(CCTouch *pTouch, CCEvent *pEvent) {
    m_activeTouch = false;
}

std::pair<size_t, cocos2d::CCPoint> CodeEditor::touchPosToBufferPos(CCPoint pos) {
    // currently pos is the space of the entire code editor, we want to convert it so that
    // 0,0 becomes the bottom left of the code label (assuming code is large enough to get to the bottom)
    pos = pos - CCPoint{TEXT_PAD + LINE_NUMBER_WIDTH, TEXT_PAD};
    if (pos.x < 0 || pos.y < 0) return {-1, {}};

    auto cl = m_scrollLayer->m_contentLayer;
    float scrollAmount = cl->getPositionY(); // positive when scrolling down (content moves up)
    pos.y -= scrollAmount;

    // now, pos.y became relative to the bottom of the ContentLayer.
    // the content layer itself is either the height of the label or the minimum cap which is scroll layer height
    CCPoint labelRelPos = pos;

    float labelHeight = cl->getScaledContentHeight();
    if (labelHeight < cl->getContentHeight()) {
        // if the user clicked on the empty space at the bottom, snap the Y position to the last line in the label
        float distToTop = cl->getContentHeight() - pos.y;
        if (distToTop >= labelHeight) {
            pos.y = cl->getContentHeight() - labelHeight;
        }

        // convert position to be relative to the label
        labelRelPos.y -= (cl->getContentHeight() - labelHeight);
    } else {
        // otherwise, position remains as is!
    }

    log::debug("Touch pos: {}, relative to label: {}", pos, labelRelPos);

    // TODO: this logic really could be improved
    // find the closest character
    CCSprite* spr = nullptr;
    float distance = 99999999.f;

    for (auto label : m_textLabels) {
        for (auto child : label->getChildrenExt<CCSprite>()) {
            float dist = child->getPosition().getDistance(labelRelPos);
            if (dist < distance) {
                distance = dist;
                spr = child;
            }
        }
    }

    log::debug("Closest: {} (distance {})", spr, distance);

    if (!spr) {
        return {-1, {}};
    }

    // find if it's closer to the start or the end of the character
    CCPoint left = spr->getPosition() - spr->getContentWidth() / 2.f;
    CCPoint right = spr->getPosition() - -spr->getContentWidth() / 2.f;

    bool leftCloser = left.getDistance(labelRelPos) < right.getDistance(labelRelPos);

    // auto cursorPos = m_label->getPosition() - CCPoint{0.f, labelHeight} + (leftCloser ? left : right);
    // size_t bufPos = spr->getZOrder() + (leftCloser ? 0 : 1);

    // return {bufPos, cursorPos};
    return {};
}

CodeEditor* CodeEditor::create(CCSize size) {
    auto ret = new CodeEditor;
    if (ret->init(size)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

}