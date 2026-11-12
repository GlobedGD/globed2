#pragma once

#include <globed/prelude.hpp>
#include <globed/util/singleton.hpp>
#include <globed/core/data/PlayerIconData.hpp>
#include <asp/time/Instant.hpp>
#include <asp/sync/SpinLock.hpp>
#include <Geode/utils/function.hpp>
#include <AsyncLoad/Manager.hpp>
#include <AsyncLoad/TaskGroup.hpp>

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

struct PreloadItem {
    asp::BoxedString image;
    IconType iconType;
    int iconId;

    PreloadItem(asp::BoxedString image, IconType iconType, int iconId)
        : image(std::move(image)), iconType(iconType), iconId(iconId) {}

    PreloadItem(PreloadItem&&) noexcept = default;
    PreloadItem& operator=(PreloadItem&&) noexcept = default;
};

TextureQuality getTextureQuality();

// An enum describing *where* are we preloading assets
enum class PreloadContext {
    None,       // None
    Loading,    // During the initial loading phase
    Reloading,  // During a texture reload
    Level,      // When loading into a level
};

struct PreloadProgress {
    size_t totalLoaded;
    size_t totalCount;
    size_t batchLoaded;
    size_t batchSize;
};

class PreloadManager : public SingletonBase<PreloadManager> {
public:
    using Callback = Function<void(Result<>)>;

    AsyncLoad::TaskGroup enterContext(PreloadContext context);
    void exitContext();

    // Submits a task to load this item and pushes into group. May complete this task eagerly and add nothing to the group.
    void submitItem(AsyncLoad::TaskGroup& group, PreloadItem item, Callback callback = {});

    // Unlike submitItem, this does not add the task into a group. It also may complete eagerly and return a null handle.
    AsyncLoad::TaskHandle submitItemIndependent(PreloadItem item, Callback callback = {});

    // Loads the given icons asynchronously
    // This may complete eagerly, and invoke the callback immediately
    void loadIcons(PlayerIconData icons, Callback callback);
    // Loads the given icon asynchronously
    // This may complete eagerly, and invoke the callback immediately
    void loadIcon(IconType ty, int icon, Callback callback);

    /// Returns whether all death effects have been loaded
    bool deathEffectsLoaded();
    /// Returns whether a specific death effect has been loaded
    bool deathEffectLoaded(int id);
    /// Returns whether all icons have been loaded
    bool areAllIconsLoaded();
    /// Returns whether this icon has been loaded
    bool isIconLoaded(IconType ty, int id);

    cocos2d::CCTexture2D* getCachedIcon(int iconType, int id);
    void setCachedIcon(int iconType, int id, geode::Ref<cocos2d::CCTexture2D> texture);

    /// Returns available system memory in bytes
    uint64_t getAvailableMemory();

    /// Releases all GL textures held by the manager. This must be called when recreating GL context.
    void releaseTextures();

private:
    friend class SingletonBase;

    PreloadContext m_context = PreloadContext::None;

    bool m_allIconsLoaded = false;
    bool m_deathEffectsLoaded = false;
    asp::SpinLock<geode::utils::StringSet> m_loadedFrames;
    std::unordered_map<std::pair<int, int>, geode::Ref<cocos2d::CCTexture2D>, PairIntIntHash> m_loadedIcons;
    std::unordered_set<int> m_loadedDeathEffects;

    PreloadManager();
    ~PreloadManager();

    AsyncLoad::TaskGroup initLoadQueue();

    Result<> onSheetLoaded(Result<AsyncLoad::LoadedSpritesheet> result, IconType iconType, int iconId);
};

}
