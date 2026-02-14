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
    auto layout = static_cast<SimpleColumnLayout*>(this->getLayout());
    layout->setMainAxisDirection(layout->getMainAxisDirection() == AxisDirection::TopToBottom ? AxisDirection::BottomToTop : AxisDirection::TopToBottom);
    this->updateLayout();
    return this;
}

RowContainer* RowContainer::create(float gap) {
    auto ret = new RowContainer();
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
    auto layout = static_cast<SimpleRowLayout*>(this->getLayout());
    layout->setMainAxisDirection(layout->getMainAxisDirection() == AxisDirection::LeftToRight ? AxisDirection::RightToLeft : AxisDirection::LeftToRight);
    this->updateLayout();
    return this;
}

}