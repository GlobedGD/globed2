#pragma once
#include <defs.hpp>
#include <data/types/misc.hpp>

class GlobedLevelListLayer : public cocos2d::CCLayer, LevelManagerDelegate {
public:
    static constexpr float LIST_WIDTH = 358.f;
    static constexpr float LIST_HEIGHT = 220.f;

    static GlobedLevelListLayer* create();
    ~GlobedLevelListLayer();

private:
    enum class LoadingState {
        Idle = 0,           // not doing anything
        WaitingServer = 1,  // waiting for the game server
        WaitingRobtop = 2,  // waiting for the robtop server
    };

    GJListLayer* listLayer = nullptr;
    LoadingCircle* loadingCircle = nullptr;
    std::unordered_map<int, unsigned short> levelList;
    LoadingState loadingState = LoadingState::Idle;

    bool init() override;
    void keyBackClicked() override;
    void refreshLevels();
    void onLevelsReceived();

    void loadListCommon();

	void loadLevelsFinished(cocos2d::CCArray*, char const*) override;
	void loadLevelsFailed(char const*) override;
	void loadLevelsFinished(cocos2d::CCArray*, char const*, int) override;
	void loadLevelsFailed(char const*, int) override;
	void setupPageInfo(gd::string, char const*) override;
};