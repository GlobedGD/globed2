#include "cocos.hpp"

#include <defs/geode.hpp>
#include <managers/settings.hpp>
#include <hooks/game_manager.hpp>
#include <util/format.hpp>
#include <util/debug.hpp>

using namespace geode::prelude;

constexpr size_t THREAD_COUNT = 25;

// all of this is needed to disrespect the privacy of ccfileutils
#include <Geode/modify/CCFileUtils.hpp>
class $modify(HookedFileUtils, CCFileUtils) {
    gd::string& getSearchPath(size_t idx) {
        return m_searchPathArray.at(idx);
    }

    gd::map<gd::string, gd::string>& getFullPathCache() {
        return m_fullPathCache;
    }

    // make these 2 public

    gd::string pGetNewFilename(const char* pfn) {
        return this->getNewFilename(pfn);
    }

    gd::string pGetPathForFilename(const gd::string& filename, const gd::string& resdir, const gd::string& sp) {
        return this->getPathForFilename(filename, resdir, sp);
    }

    static HookedFileUtils& get() {
        return *static_cast<HookedFileUtils*>(CCFileUtils::sharedFileUtils());
    }
};

namespace util::cocos {
    // big hack to call a private cocos function
    namespace {
        template <typename TC>
        using priv_method_t = void(TC::*)(CCDictionary*, CCTexture2D*);

        template <typename TC, priv_method_t<TC> func>
        struct priv_caller {
            friend void _addSpriteFramesWithDictionary(CCDictionary* p1, CCTexture2D* p2) {
                auto* obj = CCSpriteFrameCache::sharedSpriteFrameCache();
                (obj->*func)(p1, p2);
            }
        };

        template struct priv_caller<CCSpriteFrameCache, &CCSpriteFrameCache::addSpriteFramesWithDictionary>;

        void _addSpriteFramesWithDictionary(CCDictionary* p1, CCTexture2D* p2);
    }

    struct PersistentPreloadState {
        TextureQuality texQuality;
        bool hasTexturePack;
        size_t gameSearchPathIdx = -1;
        std::vector<size_t> texturePackIndices;
    };

    static void initPreloadState(PersistentPreloadState& state) {
        auto startTime = util::time::now();

        state.texturePackIndices.clear();

        state.texQuality = getTextureQuality();

        std::filesystem::path resourceDir(geode::dirs::getGameDir());
        resourceDir /= "Resources";

        size_t idx = 0;
        for (const auto& path : CCFileUtils::get()->getSearchPaths()) {
            std::string_view sv(path);
            if (sv.find("geode.texture-loader") != std::string::npos && sv.find("unzipped") == std::string::npos) {
                state.hasTexturePack = true;
                state.texturePackIndices.push_back(idx);
            }

#ifdef GEODE_IS_ANDROID
            if (sv == "assets/") {
                state.gameSearchPathIdx = idx;
            }
#elif defined(GEODE_IS_MACOS)
            if (sv.empty()) {
                state.gameSearchPathIdx = idx;
            }
#else
            auto fspath = std::filesystem::path(std::string(path));

            std::error_code ec;
            bool result = std::filesystem::equivalent(fspath, resourceDir, ec);

            if (ec == std::error_code{} && result) {
                state.gameSearchPathIdx = idx;
            }
#endif

            idx++;
        }

        log::debug("initialized preload state in {}", util::format::formatDuration(util::time::now() - startTime));
        log::debug("texture quality: {}", state.texQuality == TextureQuality::High ? "High" : (state.texQuality == TextureQuality::Medium ? "Medium" : "Low"));
        log::debug("texure packs: {}", state.texturePackIndices.size());
        log::debug("game resources path ({}): {}", state.gameSearchPathIdx,
            state.gameSearchPathIdx == -1 ? "<not found>" : HookedFileUtils::get().getSearchPath(state.gameSearchPathIdx));
    }

    void loadAssetsParallel(const std::vector<std::string>& images) {
#ifndef GEODE_IS_MACOS
        static sync::ThreadPool threadPool(THREAD_COUNT);
#else
        // macos is stupid so we leak the pool.
        // otherwise we get an epic SIGABRT in std::thread::~thread during threadpool destruction
        // TODO: might be our fault though
        static sync::ThreadPool* threadPoolPtr = new sync::ThreadPool(THREAD_COUNT);

        auto& threadPool = *threadPoolPtr;
#endif

        static sync::WrappingMutex<void> cocosWorkMutex;

        auto textureCache = CCTextureCache::sharedTextureCache();
        auto sfCache  = CCSpriteFrameCache::sharedSpriteFrameCache();

        auto& fileUtils = HookedFileUtils::get();

        struct ImageLoadState {
            std::string key;
            gd::string path;
            CCTexture2D* texture = nullptr;
        };

        sync::WrappingMutex<std::vector<ImageLoadState>> imgStates;

        auto _imguard = imgStates.lock();
        for (const auto& imgkey : images) {
            auto pathKey = fmt::format("{}.png", imgkey);

            gd::string fullpath = fullPathForFilename(pathKey);

            if (fullpath.empty()) {
                continue;
            }

            if (textureCache->m_pTextures->objectForKey(fullpath) != nullptr) {
                continue;
            }

            _imguard->emplace_back(ImageLoadState {
                .key = imgkey,
                .path = fullpath,
                .texture = nullptr
            });
        }

        size_t imgCount = _imguard->size();
        _imguard.unlock();

        sync::SmartMessageQueue<std::pair<size_t, CCImage*>> textureInitRequests;

        for (size_t i = 0; i < imgCount; i++) {
            threadPool.pushTask([i, &fileUtils, &textureInitRequests, &imgStates] {
                // this is a dangling reference, but we do not modify imgStates in any way, so it's not a big deal.
                auto& imgState = imgStates.lock()->at(i);

                // on android, resources are read from the apk file, so it's NOT thread safe. add a lock.
#ifdef GEODE_IS_ANDROID
                auto _rguard = cocosWorkMutex.lock();
#endif

                unsigned long filesize = 0;
                unsigned char* buffer = fileUtils.getFileData(imgState.path.c_str(), "rb", &filesize);

#ifdef GEODE_IS_ANDROID
                _rguard.unlock();
#endif

                std::unique_ptr<unsigned char[]> buf(buffer);

                if (!buffer || filesize == 0) {
                    log::warn("failed to read image file: {}", imgState.path);
                    return;
                }

                auto* image = new CCImage;
                if (!image->initWithImageData(buf.get(), filesize, cocos2d::CCImage::kFmtPng)) {
                    delete image;
                    log::warn("failed to init image: {}", imgState.path);
                    return;
                }

                textureInitRequests.push(std::make_pair(i, image));
            });
        }

        // initialize all the textures (must be done on the main thread)
        while (true) {
            if (textureInitRequests.empty()) {
                if (threadPool.isDoingWork()) {
                    std::this_thread::yield();
                    continue;
                } else if (textureInitRequests.empty()) {
                    break;
                }
            }

            auto [idx, image] = textureInitRequests.pop();

            auto texture = new CCTexture2D;
            if (!texture->initWithImage(image)) {
                delete texture;
                image->release();
                log::warn("failed to init CCTexture2D: {}", imgStates.lock()->at(idx).path);
                continue;
            }

            auto imguard = imgStates.lock();
            imguard->at(idx).texture = texture;
            textureCache->m_pTextures->setObject(texture, imguard->at(idx).path);
            imguard.unlock();

            texture->release();
            image->release();
        }

        // now, add sprite frames
        for (size_t i = 0; i < imgCount; i++) {
            // this is the slow code but is essentially equivalent to the code below
            // auto imgState = imgStates.lock()->at(i);
            // auto plistKey = fmt::format("{}.plist", imgState.key);
            // auto fp = CCFileUtils::sharedFileUtils()->fullPathForFilename(plistKey.c_str(), false);
            // sfCache->addSpriteFramesWithFile(fp.c_str());

            threadPool.pushTask([=, &imgStates] {
                // this is a dangling reference, but we do not modify imgStates in any way, so it's not a big deal.
                auto& imgState = imgStates.lock()->at(i);

                if (!imgState.texture) return;

                auto plistKey = fmt::format("{}.plist", imgState.key);

                {
                    auto _ = cocosWorkMutex.lock();

                    if (static_cast<HookedGameManager*>(GameManager::get())->m_fields->loadedFrames.contains(plistKey)) {
                        log::debug("already contains, skipping");
                        return;
                    }
                }

                auto pathsv = std::string_view(imgState.path);
                std::string fullPlistPath = std::string(pathsv.substr(0, pathsv.find(".png"))) + ".plist";

                // file reading is not thread safe on android.

#ifdef GEODE_IS_ANDROID
                auto _ccdlock = cocosWorkMutex.lock();
#endif

                CCDictionary* dict = CCDictionary::createWithContentsOfFileThreadSafe(fullPlistPath.c_str());
                if (!dict) {
                    log::debug("dict is nullptr for: {}, trying slower fallback option", fullPlistPath);
                    gd::string fallbackPath;
                    {
#ifndef GEODE_IS_ANDROID
                        auto _ = cocosWorkMutex.lock();
#endif
                        fallbackPath = fullPathForFilename(plistKey.c_str());
                    }

                    log::debug("attempted fallback: {}", fallbackPath);
                    dict = CCDictionary::createWithContentsOfFileThreadSafe(fallbackPath.c_str());
                }

                if (!dict) {
                    log::warn("failed to find the plist for {}.", imgState.path);

#ifndef GEODE_IS_ANDROID
                    auto _ = cocosWorkMutex.lock();
#endif

                    // remove the texture.
                    textureCache->m_pTextures->removeObjectForKey(imgState.path);
                    return;
                }

#ifdef GEODE_IS_ANDROID
                _ccdlock.unlock();
#endif

                {
                    auto _ = cocosWorkMutex.lock();

                    _addSpriteFramesWithDictionary(dict, imgState.texture);
                    static_cast<HookedGameManager*>(GameManager::get())->m_fields->loadedFrames.insert(plistKey);
                }

                dict->release();
            });
        }

        // wait for the creation of sprite frames to finish.
        threadPool.join();
    }

    void preloadAssets(AssetPreloadStage stage) {
        using BatchedIconRange = HookedGameManager::BatchedIconRange;

        auto* gm = static_cast<HookedGameManager*>(GameManager::get());

        switch (stage) {
        case AssetPreloadStage::DeathEffect: {
            std::vector<std::string> images;

            for (size_t i = 1; i < 20; i++) {
                auto key = fmt::format("PlayerExplosion_{:02}", i);
                images.push_back(key);
            }

            loadAssetsParallel(images);
        } break;
        case AssetPreloadStage::Cube: gm->loadIconsBatched((int)IconType::Cube, 0, 484); break;

        // There are actually 169 ship icons, but for some reason, loading the last icon causes
        // a very strange bug when you have the Default mini icons option enabled.
        // I have no idea how loading a ship icon can cause a ball icon to become a cube,
        // and honestly I don't care enough.
        // https://github.com/dankmeme01/globed2/issues/93
        case AssetPreloadStage::Ship: gm->loadIconsBatched((int)IconType::Ship, 1, 168); break;
        case AssetPreloadStage::Ball: gm->loadIconsBatched((int)IconType::Ball, 0, 118); break;
        case AssetPreloadStage::Ufo: gm->loadIconsBatched((int)IconType::Ufo, 1, 149); break;
        case AssetPreloadStage::Wave: gm->loadIconsBatched((int)IconType::Wave, 1, 96); break;
        case AssetPreloadStage::Other: {
            std::vector<BatchedIconRange> ranges = {
                BatchedIconRange{
                    .iconType = (int)IconType::Robot,
                    .startId = 1,
                    .endId = 68
                },
                BatchedIconRange{
                    .iconType = (int)IconType::Spider,
                    .startId = 1,
                    .endId = 69
                },
                BatchedIconRange{
                    .iconType = (int)IconType::Swing,
                    .startId = 1,
                    .endId = 43
                },
                BatchedIconRange{
                    .iconType = (int)IconType::Jetpack,
                    .startId = 1,
                    .endId = 5
                },
            };

            gm->loadIconsBatched(ranges);

        } break;
        case AssetPreloadStage::AllWithoutDeathEffects: [[fallthrough]];
        case AssetPreloadStage::All: {
            if (stage != AssetPreloadStage::AllWithoutDeathEffects) {
                preloadAssets(AssetPreloadStage::DeathEffect);
            }
            preloadAssets(AssetPreloadStage::Cube);
            preloadAssets(AssetPreloadStage::Ship);
            preloadAssets(AssetPreloadStage::Ball);
            preloadAssets(AssetPreloadStage::Ufo);
            preloadAssets(AssetPreloadStage::Wave);
            preloadAssets(AssetPreloadStage::Other);
        } break;
        }
    }

    bool forcedSkipPreload() {
        auto& settings = GlobedSettings::get();

        return !settings.globed.preloadAssets || Loader::get()->getLaunchFlag("globed-skip-preload") || Mod::get()->getSettingValue<bool>("force-skip-preload");
    }

    bool shouldTryToPreload(bool onLoading) {
        // if preloading is completely disabled, always return false
        if (forcedSkipPreload()) return false;

        auto* gm = static_cast<HookedGameManager*>(GameManager::get());

        // if already loaded, don't try again
        if (gm->getAssetsPreloaded()) {
            return false;
        }

        auto& settings = GlobedSettings::get();

        // if we are on the loading screen, only load if not deferred
        if (onLoading) {
            return !settings.globed.deferPreloadAssets;
        }

        // if we are in a level and they haven't been loaded yet, load them
        return true;
    }

    TextureQuality getTextureQuality() {
        float sf = CCDirector::get()->getContentScaleFactor();
        if (sf >= 4.f) {
            return TextureQuality::High;
        } else if (sf >= 2.f) {
            return TextureQuality::Medium;
        } else {
            return TextureQuality::Low;
        }
    }

    PersistentPreloadState& getPreloadState() {
        using util::misc::Fuse;

        static PersistentPreloadState state;
        util::misc::callOnce("cocos-get-preload-state-init", [&] {
            initPreloadState(state);
        });

        return state;
    }

    void resetPreloadState() {
        auto& state = getPreloadState();
        initPreloadState(state);
    }

    // transforms a string like "icon-41" into "icon-41-hd.png" depending on the current texture quality.
    static void appendQualitySuffix(std::string& out, TextureQuality quality, bool plist) {
        switch (quality) {
            case TextureQuality::Low: {
                if (plist) out.append(".plist");
                else out.append(".png");
            } break;
            case TextureQuality::Medium: {
                if (plist) out.append("-hd.plist");
                else out.append("-hd.png");
            } break;
            case TextureQuality::High: {
                if (plist) out.append("-uhd.plist");
                else out.append("-uhd.png");
            } break;
        }
    }

    gd::string fullPathForFilename(const std::string_view rawfilename) {
        auto& fileUtils = HookedFileUtils::get();
        auto& pstate = getPreloadState();

        // add the quality suffix.
        std::string filename;

        if (rawfilename.ends_with(".plist")) {
            filename = rawfilename.substr(0, rawfilename.find(".plist"));
            appendQualitySuffix(filename, pstate.texQuality, true);
        } else {
            filename = rawfilename.substr(0, rawfilename.find(".png"));
            appendQualitySuffix(filename, pstate.texQuality, false);
        }

        gd::string filenameGd(filename);

        // if absolute path, return it
        if (fileUtils.isAbsolutePath(filenameGd)) {
            return filename;
        }

        // if cached, return the cached path
        auto cachedIt = fileUtils.getFullPathCache().find(filenameGd);
        if (cachedIt != fileUtils.getFullPathCache().end()) {
            return cachedIt->second;
        }

        // Get the new file name.
        gd::string newFilename = fileUtils.pGetNewFilename(filename.c_str());

        std::string fullpath = "";

        const auto& searchPaths = fileUtils.getSearchPaths();
# define TRY_PATH(sp) \
    auto _fp = getPathForFilename(newFilename, sp); \
    if (!_fp.empty()) { \
        fileUtils.getFullPathCache().insert(std::make_pair(std::move(filenameGd), std::move(_fp))); \
        return _fp; \
    }

        // check for texture pack overrides
        for (size_t tpidx : pstate.texturePackIndices) {
            const auto& spath = searchPaths.at(tpidx);
            TRY_PATH(spath);
        }

        // try the gd folder
        if (pstate.gameSearchPathIdx != -1) {
            const auto& spath = searchPaths.at(pstate.gameSearchPathIdx);
            TRY_PATH(spath);
        }

        log::warn("fullPathForFilename missed known paths, trying everything: {}", rawfilename);

        // try every single search path
        for (const auto& spath : searchPaths) {
            TRY_PATH(spath);
        }

        // if all else fails, accept our defeat.
        log::warn("fullPathForFilename failed to find full path for: {}", rawfilename);
        log::warn("attempted transformed path was {}", newFilename);

        return filenameGd;
    }

    gd::string getPathForFilename(const gd::string& filename, const gd::string& searchPath) {
        // i dont wanna deal with utf8 conversions lol
        return HookedFileUtils::get().pGetPathForFilename(filename, "", searchPath);
        // std::string file(filename);
        // std::string file_path;
        // size_t slashpos = filename.find_last_of("/");
        // if (slashpos != std::string::npos) {
        //     file_path = filename.substr(0, slashpos + 1);
        //     file = filename.substr(slashpos + 1);
        // }

        // std::string path = std::string(searchPath) + file_path + file;

        // if (std::filesystem::exists(path)) {
        //     return path;
        // }

        // return "";
    }
}