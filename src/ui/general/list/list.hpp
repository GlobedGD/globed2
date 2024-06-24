#pragma once

#include <defs/geode.hpp>
#include <defs/assert.hpp>
#include <defs/util.hpp>

#include "border.hpp"
#include <util/ui.hpp>

template <typename CellType>
    requires (std::is_base_of_v<cocos2d::CCNode, CellType>)
class GlobedListCell : public cocos2d::CCLayer {
public:
    static GlobedListCell* create(CellType* inner, float width) {
        auto ret = new GlobedListCell();
        if (ret->init(inner, width)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }

    template <typename T>
    static GlobedListCell* create(CellType* inner, float width, const T& color) {
        auto* cell = create(inner, width);
        if (cell) {
            cell->setColor(color);
        }

        return cell;
    }

private:
    template <typename T>
        requires (std::is_base_of_v<cocos2d::CCNode, T>)
    friend class GlobedListLayer;

    CellType* inner;
    cocos2d::CCLayerColor* background;

    bool init(CellType* inner, float width) {
        if (!CCLayer::init()) return false;

        this->inner = inner;
        this->addChild(inner);

        float height = inner->getScaledContentSize().height;

        this->setContentSize(cocos2d::CCSize{width, height});

        Build<cocos2d::CCLayerColor>::create(cocos2d::ccColor4B{255, 255, 255, 255}, width, height)
            .zOrder(-1)
            .parent(this)
            .store(background);

        return true;
    }

    template <typename T>
    void setColor(const T& color) {
        background->setColor(globed::into<cocos2d::ccColor3B>(color));
    }

    template <>
    void setColor(const cocos2d::ccColor4B& color) {
        background->setColor(globed::into<cocos2d::ccColor3B>(color));
        background->setOpacity(color.a);
    }
};

template <typename CellType>
    requires (std::is_base_of_v<cocos2d::CCNode, CellType>)
class GlobedListLayer : public cocos2d::CCLayer {
public:
    using WrapperCell = GlobedListCell<CellType>;

    class ScrollPos {
    public:
        ScrollPos(float val) : val(val), atBottom(false) {}
        ScrollPos() : val(0.f), atBottom(true) {}

        operator float() { return val; }

        float val = 0.0f;
        bool atBottom = false;
    };

    size_t cellCount() {
        return scrollLayer->m_contentLayer->getChildrenCount();
    }

    CellType* getCell(int index) {
        GLOBED_REQUIRE(index > 0 && index < this->cellCount(), "invalid index passed to getCell");

        return static_cast<WrapperCell*>(scrollLayer->m_contentLayer->getChildren()->objectAtIndex(index))->inner;
    }

    void addCell(CellType* cell) {
        auto wcell = WrapperCell::create(cell, width);
        wcell->setColor(this->getCellColor(this->cellCount()));
        wcell->setZOrder(this->cellCount());

        scrollLayer->m_contentLayer->addChild(wcell);
        scrollLayer->m_contentLayer->updateLayout();
    }

    void insertCell(CellType* cell, int index) {
        GLOBED_REQUIRE(index > 0 && index <= this->cellCount(), "invalid index passed to insertCell");

        auto wcell = WrapperCell::create(cell, height);
        wcell->setColor(this->getCellColor(index));

        // move all cells one up
        for (int i = index; i < this->cellCount(); i++) {
            static_cast<WrapperCell*>(scrollLayer->m_contentLayer->getChildren()->objectAtIndex(i))->setZOrder(i + 1);
        }

        scrollLayer->m_contentLayer->addChild(wcell, index);
        scrollLayer->m_contentLayer->updateLayout();
    }

    void removeCell(WrapperCell* cell) {
        scrollLayer->m_contentLayer->removeChild(cell);
    }

    void removeCell(CellType* cell) {
        this->removeCell(static_cast<WrapperCell*>(cell->getParent()));
    }

    void removeCell(int index) {
        GLOBED_REQUIRE(index > 0 && index < this->cellCount(), "invalid index passed to removeCell");

        this->removeCell(this->getCell(index));
        this->updateCellOrder();
    }

    void removeAllCells() {
        scrollLayer->m_contentLayer->removeAllChildren();
    }

    void swapCells(cocos2d::CCArray* cells) {
        this->removeAllCells();

        for (auto* cell : CCArrayExt<CellType*>(cells)) {
            this->addCell(cell);
        }
    }

    void scrollToTop() {
        util::ui::scrollToTop(scrollLayer);
    }

    void scrollToBottom() {
        util::ui::scrollToBottom(scrollLayer);
    }

    ScrollPos getScrollPos() {
        auto* cl = scrollLayer->m_contentLayer;
        if (cl->getPositionY() > 0.f) return ScrollPos();
        return ScrollPos(cl->getScaledContentSize().height + cl->getPositionY());
    }

    void scrollToPos(ScrollPos pos) {
        if (pos.atBottom) return;

        auto* cl = scrollLayer->m_contentLayer;
        float actualPos = pos.val - cl->getScaledContentSize().height;

        cl->setPositionY(std::min(actualPos, 0.f));
    }

    void scrollToCell() {
        // unimpl
    }

    template <typename T>
    void setCellColor(const T& in) {
        oddCellColor = evenCellColor = globed::into<cocos2d::ccColor4B>(in);
    }

    template <typename T, typename Y>
    void setCellColors(const T& odd, const Y& even) {
        oddCellColor = globed::into<cocos2d::ccColor4B>(odd);
        evenCellColor = globed::into<cocos2d::ccColor4B>(even);
    }

    // helper templated functions

    template <typename... Args>
    CellType* addCell(Args&&... args) {
        CellType* cell = CellType::create(std::forward<Args>(args)...);
        auto wcell = WrapperCell::create(cell);

        this->addCell(wcell);

        return cell;
    }

    template <typename... Args>
    CellType* insertCell(Args&&... args, int index) {
        CellType* cell = CellType::create(std::forward<Args>(args)...);
        auto wcell = WrapperCell::create(cell);

        this->insertCell(wcell, index);

        return cell;
    }

    static GlobedListLayer* create(float width, float height, cocos2d::ccColor4B background, GlobedListBorderType borderType = GlobedListBorderType::None) {
        auto ret = new GlobedListLayer();
        if (ret->init(width, height, background, borderType)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }

protected:
    geode::ScrollLayer* scrollLayer;
    cocos2d::CCSprite *borderLeft, *borderRight, *borderTop, *borderBottom;
    float height, width;
    cocos2d::ccColor4B oddCellColor, evenCellColor;
    Ref<GlobedListBorder> borderNode;

    bool init(float width, float height, cocos2d::ccColor4B background, GlobedListBorderType borderType) {
        if (!CCLayer::init()) return false;

        this->height = height;
        this->width = width;

        Build<geode::ScrollLayer>::create(cocos2d::CCSize{width, height})
            .parent(this)
            .store(scrollLayer);

        scrollLayer->m_contentLayer->setLayout(
            cocos2d::ColumnLayout::create()
                ->setGap(0.f)
                ->setAxisReverse(true)
                ->setAxisAlignment(cocos2d::AxisAlignment::End)
                ->setAutoGrowAxis(height)
        );

        this->setContentSize({width, height});
        this->ignoreAnchorPointForPosition(false);

        // TODO: bg and separator line
        borderNode = GlobedListBorder::create(borderType, width, height, background);

        if (borderNode) {
            borderNode->setZOrder(1);
            borderNode->setAnchorPoint({0.5f, 0.5f});
            borderNode->setPosition(width / 2.f, height / 2.f);
            this->addChild(borderNode);
        }

        return true;
    }

    void updateCellOrder() {
        size_t i = 0;
        for (WrapperCell* child : CCArrayExt<WrapperCell*>(scrollLayer->m_contentLayer->getChildren())) {
            child->setColor(this->getCellColor(i));
            child->setZOrder(i++);
        }

        this->updateCells();
    }

    void updateCells() {
        for (WrapperCell* child : CCArrayExt<WrapperCell*>(scrollLayer->m_contentLayer->getChildren())) {
            child->setContentHeight(child->inner->getContentHeight());
        }

        scrollLayer->m_contentLayer->updateLayout();
    }

    cocos2d::ccColor4B getCellColor(int idx) {
        return idx % 2 == 0 ? oddCellColor : evenCellColor;
    }
};
