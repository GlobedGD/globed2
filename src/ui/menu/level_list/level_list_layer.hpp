#pragma once
#include <defs/all.hpp>
#include <data/types/misc.hpp>

class GlobedLevelListLayer : public cocos2d::CCLayer, LevelManagerDelegate {
public:
    static constexpr float LIST_WIDTH = 358.f;
    static constexpr float LIST_HEIGHT = 220.f;
    static constexpr size_t LIST_PAGE_SIZE = 100; // levels on 1 page

    static GlobedLevelListLayer* create();
    ~GlobedLevelListLayer();

private:
    GJListLayer* listLayer = nullptr;
    LoadingCircle* loadingCircle = nullptr;
    CCMenuItemSpriteExtra *btnPagePrev = nullptr, *btnPageNext = nullptr;
    std::unordered_map<int, unsigned short> levelList;
    std::vector<int> sortedLevelIds;
    std::vector<std::vector<Ref<GJGameLevel>>> levelPages;
    int currentPage = 0;
    bool loading = false;

    bool init() override;
    void keyBackClicked() override;
    void refreshLevels();
    void reloadPage();

    void loadListCommon();
    void removeLoadingCircle();
    void showLoadingUi();

	void loadLevelsFinished(cocos2d::CCArray*, char const*) override;
	void loadLevelsFailed(char const*) override;
	void loadLevelsFinished(cocos2d::CCArray*, char const*, int) override;
	void loadLevelsFailed(char const*, int) override;
	void setupPageInfo(gd::string, char const*) override;
};