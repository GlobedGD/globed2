#pragma once

#include "list_cell.hpp"
#include <defs/geode.hpp>
#include <data/types/gd.hpp>
#include <ui/general/list/list.hpp>

class RoomPlayerListPacket;
class RoomCreatedPacket;
class RoomJoinedPacket;
class RoomInfoPacket;
class RoomInfo;

class RoomLayer : public cocos2d::CCLayer, public LevelDownloadDelegate, public LevelManagerDelegate {
public:
    cocos2d::CCSize popupSize;
    cocos2d::CCSize listSize;
    cocos2d::CCSize targetButtonSize;

    static RoomLayer* create();

protected:
    friend class CreateRoomPopup;

    using PlayerList = GlobedListLayer<ListCellWrapper>;

    std::vector<PlayerRoomPreviewAccountData> playerList;
    std::string currentFilter;

    bool justEntered = true;
    Ref<LoadingCircle> loadingCircle;
    Ref<cocos2d::CCMenu> topRightButtons;
    PlayerList* listLayer;
    Ref<CCMenuItemSpriteExtra> btnSearch, btnClearSearch, btnSettings, btnInvite, btnRefresh, btnCloseRoom;
    Ref<CCMenuItemToggler> btnInvisible;
    Ref<cocos2d::CCMenu> btnRoomId, roomButtonMenu;
    Ref<ListCellWrapper> roomLevelCell;

    bool init() override;
    void update(float) override;

    // show/hide loading circle
    void startLoading();
    void stopLoading();
    bool isLoading();

    void requestPlayerList();
    void recreatePlayerList();
    void sortPlayerList();
    void setFilter(std::string_view filter);
    void setRoomTitle(std::string_view name, uint32_t id);
    void resetFilter();
    void closeRoom();

    void addRoomButtons();
    void addGlobalRoomButtons();
    void addCustomRoomButtons();

    // packet handlers
    void onPlayerListReceived(const RoomPlayerListPacket&);
    void onRoomCreatedReceived(const RoomCreatedPacket&);
    void onRoomJoinedReceived(const RoomJoinedPacket&);
    void reloadData(const RoomInfo& info, const std::vector<PlayerRoomPreviewAccountData>& players);

    // callbacks
    void onInvisibleClicked(cocos2d::CCObject*);
    void onCopyRoomId(cocos2d::CCObject*);

    // level delegate stuff for room level
    void loadLevelsFinished(cocos2d::CCArray* p0, char const* p1, int p2) override;
    void loadLevelsFinished(cocos2d::CCArray* p0, char const* p1) override;
    void loadLevelsFailed(char const* p0, int p1) override;
    void loadLevelsFailed(char const* p0) override;
    void setupPageInfo(gd::string p0, char const* p1) override;

    void levelDownloadFinished(GJGameLevel* p0) override;
    void levelDownloadFailed(int p0) override;

    void loadRoomLevel(int levelId);

    ~RoomLayer();
};
