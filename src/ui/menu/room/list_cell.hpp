#pragma once
#include <defs/all.hpp>
#include "player_list_cell.hpp"

class CollapsableLevelCell : public cocos2d::CCNode {
public:
    GJGameLevel* m_level;

    LevelCell* m_levelCell;
    CCNode* m_collapsedCell;

    bool m_isCollapsed;

    static constexpr float HEIGHT = 90.f;
    static constexpr float COLLAPSED_HEIGHT = 30.f;

    static CollapsableLevelCell* create(GJGameLevel* level, float width);

    void setIsCollapsed(bool isCollapsed);
    void onOpenLevel(cocos2d::CCObject* sender);

protected:
    bool init(GJGameLevel* level, float width);
};

// a wrapper for both PlayerListCell and CollapsableLevelCell

class ListCellWrapper : public cocos2d::CCNode {
public:
    PlayerListCell* player_cell;
    CollapsableLevelCell* room_cell;

    static ListCellWrapper* create(const PlayerRoomPreviewAccountData& data, float cellWidth, bool forInviting, bool isIconLazyLoad);
    bool init(const PlayerRoomPreviewAccountData& data, float cellWidth, bool forInviting, bool isIconLazyLoad);

    using CollapsedCallback = std::function<void(bool)>;
    static ListCellWrapper* create(GJGameLevel* level, float width, CollapsedCallback callback);
    bool init(GJGameLevel* level, float width, CollapsedCallback callback);
};
