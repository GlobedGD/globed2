#pragma once

#include <globed/util/singleton.hpp>
#include <asp/thread/ThreadPool.hpp>
#include <asp/time/Instant.hpp>
#include <asp/sync/SpinLock.hpp>
#include "Item.hpp"
#include <queue>

// TODO (very low): it's time consuming so postponing for later, but we should add background preloading,
// which allows the assets to be loaded when the game is running, rather than blocking during loading

namespace globed {

enum class TextureQuality {
    Low, Medium, High
};

// Lol
struct PairIntIntHash {
    inline size_t operator()(const std::pair<int, int>& p) const {
        size_t h1 = std::hash<int>{}(p.first);
        size_t h2 = std::hash<int>{}(p.second);
        return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
    }
};

TextureQuality getTextureQuality();

// An enum describing *where* are we preloading assets
enum class PreloadContext {
    None,       // None
    Loading,    // During the initial loading phase
    Reloading,  // During a texture reload
    Level,      // When loading into a level
};

class PreloadManager : public SingletonBase<PreloadManager> {
public:
    bool shouldPreload();
    void enterContext(PreloadContext context);
    void exitContext();

    // Loads a reasonable amount of assets in a single call, to avoid blocking the main thread for too long
    void loadNextBatch(bool blocking = true);
    // Loads all assets in a single call, blocking the main thread until all assets are loaded
    void loadEverything(bool blocking = true);

    // Returns the number of assets that have been loaded so far
    size_t getLoadedCount();
    // Returns the total number of assets that will be loaded
    size_t getTotalCount();

    /// Returns whether death effects have been loaded
    bool deathEffectsLoaded();

    cocos2d::CCTexture2D* getCachedIcon(int iconType, int id);
    void setCachedIcon(int iconType, int id, cocos2d::CCTexture2D* texture);

private:
    friend class SingletonBase;
    friend struct PreloadItemState;
    struct SessionState {
        bool initialized = false;
        bool hasTexturePack;
        TextureQuality texQuality;
        size_t gameSearchPathIdx = -1;
        std::vector<size_t> texturePackIndices;
        std::unique_ptr<asp::ThreadPool> threadPool;

        // time measurements
        asp::time::Instant preInitTime;
        asp::time::Instant postInitTime;
    };

    PreloadContext m_context = PreloadContext::None;
    std::deque<PreloadItem> m_loadQueue;
    SessionState m_sstate;
    size_t m_totalCount = 0;

    bool m_iconsLoaded = false;
    bool m_deathEffectsLoaded = false;
    asp::SpinLock<geode::utils::StringSet> m_loadedFrames;
    std::unordered_map<std::pair<int, int>, geode::Ref<cocos2d::CCTexture2D>, PairIntIntHash> m_loadedIcons;

    PreloadManager();
    ~PreloadManager();

    void resetState();
    void initLoadQueue();
    void doLoadBatch(std::vector<PreloadItem> items, bool blocking);
    void initSessionState();

    gd::string fullPathForFilename(std::string_view input, bool ignoreSuffix = false);
};

}
