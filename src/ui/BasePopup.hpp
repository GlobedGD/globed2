#pragma once

#include <Geode/ui/Popup.hpp>
#include <Geode/binding/GameManager.hpp>
#include <globed/util/singleton.hpp>

namespace globed {

class GLOBED_NOVTABLE BasePopup : public geode::Popup {
public:
    BasePopup() = default;
    BasePopup(const BasePopup&) = delete;
    BasePopup& operator=(const BasePopup&) = delete;
    BasePopup(BasePopup&&) = delete;
    BasePopup& operator=(BasePopup&&) = delete;

protected:
    bool init(
        float width, float height, char const* bg = "GJ_square01.png",
        cocos2d::CCRect bgRect = {}
    ) {
        if (cachedSingleton<GameManager>()->getGameVariable("0168")) {
            m_noElasticity = true;
        }
        return Popup::init(width, height, bg, bgRect);
    }

    bool init(cocos2d::CCSize size, char const* bg = "GJ_square01.png", cocos2d::CCRect bgRect = {}) {
        return init(size.width, size.height, bg, bgRect);
    }

    // various helper methods for positioning ui elements
    float left() { return 0.f; }
    float right() { return this->m_mainLayer->getContentSize().width; }
    float top() { return this->m_mainLayer->getContentSize().height; }
    float bottom() { return 0.f; }
    float centerX() { return this->right() / 2.f; }
    float centerY() { return this->top() / 2.f; }
    cocos2d::CCPoint center() { return { this->centerX(), this->centerY() }; }
    cocos2d::CCPoint centerLeft() { return { this->left(), this->centerY() }; }
    cocos2d::CCPoint centerRight() { return { this->right(), this->centerY() }; }
    cocos2d::CCPoint centerTop() { return { this->centerX(), this->top() }; }
    cocos2d::CCPoint centerBottom() { return { this->centerX(), this->bottom() }; }
    cocos2d::CCPoint bottomLeft() { return { this->left(), this->bottom() }; }
    cocos2d::CCPoint bottomRight() { return { this->right(), this->bottom() }; }
    cocos2d::CCPoint topLeft() { return { this->left(), this->top() }; }
    cocos2d::CCPoint topRight() { return { this->right(), this->top() }; }

    cocos2d::CCPoint fromTop(float y) { return fromTop(cocos2d::CCSize{0.f, y}); }
    cocos2d::CCPoint fromTop(cocos2d::CCSize off) {
        return this->centerTop() + cocos2d::CCPoint{off.width, -off.height};
    }

    cocos2d::CCPoint fromBottom(float y) { return fromBottom(cocos2d::CCSize{0.f, y}); }
    cocos2d::CCPoint fromBottom(cocos2d::CCSize off) {
        return this->centerBottom() + off;
    }

    cocos2d::CCPoint fromLeft(float x) { return fromLeft(cocos2d::CCSize{x, 0.f}); }
    cocos2d::CCPoint fromLeft(cocos2d::CCSize off) {
        return this->centerLeft() + off;
    }

    cocos2d::CCPoint fromRight(float x) { return fromRight(cocos2d::CCSize{x, 0.f}); }
    cocos2d::CCPoint fromRight(cocos2d::CCSize off) {
        return this->centerRight() + cocos2d::CCPoint{-off.width, off.height};
    }

    cocos2d::CCPoint fromCenter(float x, float y) { return fromCenter({x, y}); }
    cocos2d::CCPoint fromCenter(cocos2d::CCSize off) {
        return this->center() + off;
    }

    cocos2d::CCPoint fromBottomRight(float x, float y) { return fromBottomRight({x, y}); }
    cocos2d::CCPoint fromBottomRight(cocos2d::CCSize off) {
        return this->bottomRight() + cocos2d::CCPoint{-off.width, off.height};
    }

    cocos2d::CCPoint fromTopRight(float x, float y) { return fromTopRight({x, y}); }
    cocos2d::CCPoint fromTopRight(cocos2d::CCSize off) {
        return this->topRight() - off;
    }

    cocos2d::CCPoint fromBottomLeft(float x, float y) { return fromBottomLeft({x, y}); }
    cocos2d::CCPoint fromBottomLeft(cocos2d::CCSize off) {
        return this->bottomLeft() + off;
    }

    cocos2d::CCPoint fromTopLeft(float x, float y) { return fromTopLeft({x, y}); }
    cocos2d::CCPoint fromTopLeft(cocos2d::CCSize off) {
        return this->topLeft() + cocos2d::CCPoint{off.width, -off.height};
    }
};

}
