#include "Core.hpp"
#include <globed/util/FunctionQueue.hpp>

using namespace geode::prelude;

namespace globed {

static void deferredUpdate(CCNode* node) {
    FunctionQueue::get().queue([self = WeakRef(node)] {
        if (auto ptr = self.lock()) {
            ptr->updateLayout();
        }
    });
}

ColumnContainer* ColumnContainer::create(float gap) {
    auto ret = new ColumnContainer();
    ret->init();
    ret->ignoreAnchorPointForPosition(false);
    ret->setLayout(
        SimpleColumnLayout::create()
            ->setGap(gap)
            ->setMainAxisScaling(AxisScaling::Fit)
            ->setCrossAxisScaling(AxisScaling::Fit)
    );
    ret->autorelease();
    // deferredUpdate(ret);
    return ret;
}

ColumnContainer* ColumnContainer::flip() {
    auto layout = this->layout();
    layout->setMainAxisDirection(layout->getMainAxisDirection() == AxisDirection::TopToBottom ? AxisDirection::BottomToTop : AxisDirection::TopToBottom);
    this->updateLayout();
    return this;
}

SimpleColumnLayout* ColumnContainer::layout() {
    return static_cast<SimpleColumnLayout*>(this->getLayout());
}

RowContainer* RowContainer::create(float gap) {
    auto ret = new RowContainer();
    ret->init();
    ret->ignoreAnchorPointForPosition(false);
    ret->setLayout(
        SimpleRowLayout::create()
            ->setGap(gap)
            ->setMainAxisScaling(AxisScaling::Fit)
            ->setCrossAxisScaling(AxisScaling::Fit)
    );
    ret->autorelease();
    // deferredUpdate(ret);
    return ret;
}

RowContainer* RowContainer::flip() {
    auto layout = this->layout();
    layout->setMainAxisDirection(layout->getMainAxisDirection() == AxisDirection::LeftToRight ? AxisDirection::RightToLeft : AxisDirection::LeftToRight);
    this->updateLayout();
    return this;
}

SimpleRowLayout* RowContainer::layout() {
    return static_cast<SimpleRowLayout*>(this->getLayout());
}

}