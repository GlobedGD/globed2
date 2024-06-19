#include "misc.hpp"

#include <util/format.hpp>

using namespace geode::prelude;

Result<RichColor> RichColor::parse(std::string_view k) {
    if (k.find('>') == std::string::npos) {
        auto col = util::format::parseColor(k);
        if (!col) return Err(std::move(col.unwrapErr()));

        return Ok(RichColor(col.unwrap()));
    }

    if (k.starts_with('#')) {
        k = k.substr(1, k.size() - 1);
    }

    auto pieces = util::format::split(k, ">");
    std::vector<cocos2d::ccColor3B> colors;

    for (auto piece : pieces) {
        auto col = util::format::parseColor(util::format::trim(piece));
        if (!col) {
            return Err(std::move(col.unwrapErr()));
        }

        colors.emplace_back(col.unwrap());
    }

    return Ok(RichColor(colors));
}

bool RichColor::isMultiple() const {
    return inner.isSecond();
}

std::vector<cocos2d::ccColor3B>& RichColor::getColors() {
    GLOBED_REQUIRE(inner.isSecond(), "calling RichColor::getColors when there is only 1 color");

    return inner.secondRef()->get();
}

const std::vector<cocos2d::ccColor3B>& RichColor::getColors() const {
    GLOBED_REQUIRE(inner.isSecond(), "calling RichColor::getColors when there is only 1 color");

    return inner.secondRef()->get();
}

cocos2d::ccColor3B RichColor::getColor() const {
    GLOBED_REQUIRE(inner.isFirst(), "calling RichColor::getColor when there are multiple colors");

    return inner.firstRef()->get();
}

cocos2d::ccColor3B RichColor::getAnyColor() const {
    if (inner.isFirst()) {
        return inner.firstRef()->get();
    } else {
        auto& colors = inner.secondRef()->get();

        if (colors.empty()) {
            return cocos2d::ccc3(255, 255, 255);
        } else {
            return colors.at(0);
        }
    }
}

void RichColor::animateLabel(cocos2d::CCLabelBMFont* label) const {
    constexpr int tag = 34925671;

    if (!label) return;

    label->stopActionByTag(tag);

    if (!this->isMultiple()) {
        label->setColor(this->getColor());
        return;
    }

    const auto& colors = this->getColors();
    if (colors.empty()) {
        return;
    }

    // set the last color
    label->setColor(colors.at(colors.size() - 1));

    // create an action to tint between the rest of the colors
    CCArray* actions = CCArray::create();

    for (const auto& color : colors) {
        actions->addObject(CCTintTo::create(0.8f, color.r, color.g, color.b));
    }

    CCRepeat* action = CCRepeat::create(CCSequence::create(actions), 99999999);
    action->setTag(tag);

    label->runAction(action);
}