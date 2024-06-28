#include "content_layer.hpp"

#include "list.hpp"

void GlobedContentLayer::setPosition(const cocos2d::CCPoint& pos) {
    auto list = static_cast<GlobedListLayer<cocos2d::CCNode>*>(this->getParent()->getParent());

    bool disableOverscroll = list->isDisableOverScrollUp;

    auto lastPos = this->getPosition();

    CCLayerColor::setPosition(pos);

    // I could not figure this out
    // log::debug("pos: {}, bottom: {}, height: {}", list->getScrollPos().val, list->getScrollPos().atBottom, this->getScaledContentHeight());

    // auto sp = list->getScrollPos();
    // if (disableOverscroll && sp.val < this->getScaledContentHeight() && !sp.atBottom) {
    //     CCLayerColor::setPosition(lastPos);
    //     return;
    // }

    for (auto child : CCArrayExt<CCNode*>(m_pChildren)) {
        auto y = this->getPositionY() + child->getPositionY();
        float childHeight = child->getScaledContentHeight();

        // the change here from the original is `+ childHeight`
        child->setVisible(!(((m_obContentSize.height + childHeight) < y) || (y < -child->getContentSize().height)));
    }
}
