#pragma once
#include <defs/platform.hpp>
#include <cocos2d.h>
#include <Geode/c++stl/string.hpp>

namespace util::cocos {
    // Loads the given images in separate threads, in parallel. Blocks the thread until all images have been loaded.
    // This will ONLY load .png images.
    GLOBED_DLL void loadAssetsParallel(const std::vector<std::string>& images);

    enum class AssetPreloadStage {
        DeathEffect,
        Cube,
        Ship,
        Ball,
        Ufo,
        Wave,
        Other,
        All, // all at once
        AllWithoutDeathEffects,
    };

    void preloadLogImpl(std::string_view message);

    GLOBED_DLL void preloadAssets(AssetPreloadStage stage);

    bool forcedSkipPreload();
    bool shouldTryToPreload(bool onLoading);

    enum class TextureQuality {
        Low, Medium, High
    };

    TextureQuality getTextureQuality();

    // State that persists between multiple calls to `loadAssetsParallel`, but will be reset upon game reloads (i.e. changing graphics settings or texture packs)
    struct PersistentPreloadState;
    PersistentPreloadState& getPreloadState();
    void resetPreloadState();
    void cleanupThreadPool();

    ::gd::string fullPathForFilename(std::string_view filename);

    // Like cocos' func, returns empty string if file doesn't exist.
    ::gd::string getPathForFilename(const ::gd::string& filename, const ::gd::string& searchPath);

    unsigned char* getFileDataThreadSafe(const char* path, const char* mode, unsigned long* outSize);

    std::string spr(std::string_view s);

    // creates a new, independent texture
    cocos2d::CCTexture2D* textureFromSpriteName(std::string_view name);

    cocos2d::CCTexture2D* addTextureFromData(const std::string& textureKey, unsigned char* data, size_t size);

    // checks for nullptr and textureldr fallback
    bool isValidSprite(cocos2d::CCSprite* obj);

    void renderNodeToFile(cocos2d::CCNode*, const std::filesystem::path& dest);

    std::string parentChain(cocos2d::CCNode*);

    GLOBED_DLL void tryLoadDeathEffect(int id);

    // ccobject that stores a custom struct
    template <typename T>
    class CCData : public cocos2d::CCObject {
        T inner;

        CCData(T data) : inner(std::move(data)) {}

        template <typename... Args>
        CCData(Args&&... args) : inner(std::forward<Args>(args)...) {}

    public:
        template <typename... Args>
        static CCData* create(Args&&... args) {
            auto ret = new CCData{std::forward<Args>(args)...};
            ret->autorelease();
            return ret;
        }

        T& data() {
            return inner;
        }

        T* operator->() const {
            return &inner;
        }

        T& operator*() const {
            return inner;
        }
    };

    // ccobject with pointer value
    template <typename T = void>
    class CCPointer : public cocos2d::CCObject {
        T* inner;

        CCPointer(T* ptr) : inner(ptr) {}

    public:
        static CCPointer* create(T* ptr) {
            auto ret = new CCPointer{ptr};
            ret->autorelease();
            return ret;
        }

        T* get() {
            return inner;
        }

        T* operator*() const {
            return inner;
        }
    };
}