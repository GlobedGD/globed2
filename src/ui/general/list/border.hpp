#pragma once

#include <defs/geode.hpp>
#include <defs/util.hpp>

enum class GlobedListBorderType {
    None,
    GJCommentListLayer,
    GJCommentListLayerBlue,
    GJListLayer,
};

class GlobedListBorder : public cocos2d::CCNode {
public:
    using Type = GlobedListBorderType;

    template <typename T>
    static GlobedListBorder* create(Type type, float width, float height, const T& bgColor) {
        auto ret = new GlobedListBorder();
        if (ret->init(type, width, height, globed::into(bgColor))) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }

    template <typename T>
    static GlobedListBorder* createForComments(float width, float height, const T& color) {
        return create(Type::GJCommentListLayer, width, height, color);
    }

    template <typename T>
    static GlobedListBorder* createForCommentsBlue(float width, float height, const T& color) {
        return create(Type::GJCommentListLayerBlue, width, height, color);
    }

protected:
    cocos2d::ccColor4B bgColor;
    Ref<cocos2d::extension::CCScale9Sprite> borderSprite;

    bool init(Type type, float width, float height, cocos2d::ccColor4B bgColor);
};