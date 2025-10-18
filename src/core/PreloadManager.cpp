#include "PreloadManager.hpp"
#include <globed/prelude.hpp>
#include <globed/util/SpinLock.hpp>
#include <globed/core/SettingsManager.hpp>
#include <asp/fs.hpp>

#ifndef GEODE_IS_WINDOWS
# include <sys/stat.h>
#endif

using namespace asp::time;
using namespace geode::prelude;
namespace fs = std::filesystem;

namespace globed {

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

struct HookedFileUtils : public CCFileUtils {
    static HookedFileUtils& get() {
        return *static_cast<HookedFileUtils*>(CCFileUtils::get());
    }

    bool fileExists(const char* path) {
#ifdef GEODE_IS_WINDOWS
        auto attrs = GetFileAttributesA(path);

        return (attrs != INVALID_FILE_ATTRIBUTES &&
                !(attrs & FILE_ATTRIBUTE_DIRECTORY));
#else
        struct stat st;
        return (stat(path, &st) == 0) && S_ISREG(st.st_mode);
#endif
    }

    $override
    gd::string getPathForFilename(const gd::string& filename, const gd::string& resolutionDirectory, const gd::string& searchPath) {
        std::string_view file = filename;
        std::string_view filePath;

        size_t slashPos = file.find_last_of('/');
        if (slashPos != std::string::npos) {
            filePath = file.substr(0, slashPos + 1);
            file = file.substr(slashPos + 1);
        }

        std::array<char, 1024> buf;
        auto result = fmt::format_to_n(buf.data(), buf.size(), "{}{}{}{}", searchPath, filePath, resolutionDirectory, file);
        if (result.size < buf.size()) {
            buf[result.size] = '\0';
        } else {
            buf[buf.size() - 1] = '\0';
        }

        if (this->fileExists(buf.data())) {
            return gd::string(buf.data(), result.size);
        } else {
            return gd::string{};
        }
    }
};

static std::string_view relativizeIconPath(std::string_view fullpath) {
    auto countSlashes = [](std::string_view sv) {
        return std::count_if(sv.begin(), sv.end(), [](char c) { return c == '/' || c == '\\'; });
    };

    while (countSlashes(fullpath) > 1) {
        auto slashPos = fullpath.find_first_of("/\\");
        GLOBED_ASSERT(slashPos != std::string::npos);

        fullpath = fullpath.substr(slashPos + 1);
    }

    // now there is <= 1 slash! if this is the `icons/` subfolder, keep it
    // otherwise remove that part
    if (fullpath.starts_with("icons/") || fullpath.starts_with("icons\\")) {
        return fullpath;
    } else {
        auto slashPos = fullpath.find_first_of("/\\");
        if (slashPos != fullpath.npos) {
            return fullpath.substr(slashPos + 1);
        } else {
            return fullpath;
        }
    }
}

// mutex that guards ccfileutils file reading
static asp::Mutex<> g_fileMutex;

PreloadManager::PreloadManager() {}

PreloadManager::~PreloadManager() {}

void PreloadManager::initLoadQueue() {
    auto gm = globed::cachedSingleton<GameManager>();
    m_totalCount = 0;
    m_loadQueue = {};

    if (!globed::setting<bool>("core.preload.enabled")) {
        // preloading is disabled, skip everything
        return;
    }

    if (globed::setting<bool>("core.preload.defer")) {
        // preloading is deferred, only load if we are loading a level
        if (m_context != PreloadContext::Level) {
            return;
        }
    }

    // Death effects
    // skip if death effects are disabled in settings or default death effects are enabled
    bool loadDe = globed::setting<bool>("core.player.death-effects") && !globed::setting<bool>("core.player.default-death-effects");
    if (!m_deathEffectsLoaded && loadDe) {
        m_deathEffectsLoaded = true;

        for (size_t i = 1; i < 20; i++) {
            m_loadQueue.push(Item { fmt::format("PlayerExplosion_{:02}.png", i) });
        }
    }

    if (!m_iconsLoaded) {
        m_iconsLoaded = true;

        auto addIcons = [&](IconType ty, int startIdx, int endIdx) {
            for (int id = startIdx; id <= endIdx; id++) {
                auto sheetName = gm->sheetNameForIcon(id, (int)ty);
                if (sheetName.empty()) {
                    continue;
                }

                m_loadQueue.push(Item {
                    .image = sheetName,
                    .isIcon = true,
                    .iconType = ty,
                    .iconId = id
                });
            }
        };

        addIcons(IconType::Cube, 0, 485);

        // There are actually 169 ship icons, but for some reason, loading the last icon causes
        // a very strange bug when you have the Default mini icons option enabled.
        // I have no idea how loading a ship icon can cause a ball icon to become a cube,
        // and honestly I don't care enough.
        // https://github.com/GlobedGD/globed2/issues/93
        addIcons(IconType::Ship, 1, 168);
        addIcons(IconType::Ball, 0, 118);
        addIcons(IconType::Ufo, 1, 149);
        addIcons(IconType::Wave, 1, 96);
        addIcons(IconType::Robot, 1, 68);
        addIcons(IconType::Spider, 1, 69);
        addIcons(IconType::Swing, 1, 43);
        addIcons(IconType::Jetpack, 1, 8);
    }

    m_totalCount = m_loadQueue.size();
}

void PreloadManager::loadNextBatch() {
    // 200 icons is a somewhat reasonable batch to load in a single call that shouldn't cause freezes even on low-end devices
    // death effects are huge and would be counted as 20 icons
    constexpr size_t Limit = 200;
    size_t count = 0;

    std::vector<Item> items;

    while (count < Limit && !m_loadQueue.empty()) {
        items.push_back(std::move(m_loadQueue.front()));
        m_loadQueue.pop();

        auto& item = items.back();
        if (item.isIcon) {
            count++;
        } else {
            count += 20;
        }
    }

    this->doLoadBatch(items);
}

void PreloadManager::loadEverything() {
    std::vector<Item> items;
    items.reserve(m_loadQueue.size());

    while (!m_loadQueue.empty()) {
        items.push_back(std::move(m_loadQueue.front()));
        m_loadQueue.pop();
    }

    this->doLoadBatch(items);
}

void PreloadManager::doLoadBatch(std::vector<Item>& items) {
    // TODO: optimize.

    if (!m_sstate.initialized) {
        this->initSessionState();
    }

    auto timeBegin = Instant::now();

    auto& pool = *m_sstate.threadPool;

    log::debug("PreloadManager: loading batch of {} items", items.size());

    static SpinLock cocosLock;

    auto texCache = CCTextureCache::get();
    auto sfCache = CCSpriteFrameCache::get();
    auto fu = CCFileUtils::get();

    struct ItemState {
        Item& item;
        gd::string path;
        CCTexture2D* texture = nullptr;
    };

    // note: this vector might seem not thread safe and you would be right to think so,
    // although after the loop nothing is ever added/removed from it,
    // and a single specific item will initially be only accessed by the pool, and then given to the main thread
    std::vector<ItemState> itemStates;

    for (auto& item : items) {
        auto fullPath = this->fullPathForFilename(item.image + ".png");
        if (fullPath.empty()) {
            log::warn("PreloadManager: could not find full path for '{}'", item.image);
            continue;
        }

        if (texCache->m_pTextures->objectForKey(fullPath)) {
            // texture already loaded, skip
            continue;
        }

        itemStates.push_back(ItemState{
            .item = item,
            .path = std::move(fullPath),
        });
    }

    if (itemStates.empty()) return;

    log::debug("PreloadManager: loading {} images (took {} to populate)", itemStates.size(), timeBegin.elapsed().toString());
    auto timePreInit = Instant::now();

    asp::Channel<std::pair<size_t, CCImage*>> texInitRequests;

    for (size_t i = 0; i < itemStates.size(); i++) {
        pool.pushTask([i, &itemStates, &texInitRequests] {
            auto& state = itemStates[i];

            unsigned long filesize = 0;
            std::unique_ptr<unsigned char[]> buffer{getFileDataThreadSafe(state.path.c_str(), "rb", &filesize)};

            if (!buffer || filesize == 0) {
                log::warn("PreloadManager: could not read file '{}'", state.path);
                return;
            }

            auto image = new CCImage();
            if (!image->initWithImageData(buffer.get(), filesize, cocos2d::CCImage::kFmtPng)) {
                delete image;
                log::warn("PreloadManager: failed to init image '{}'", state.path);
                return;
            }

            texInitRequests.push({i, image});
        });
    }

    // initialize all the textures, which must be done on the main thread
    size_t inited = 0;
    while (true) {
        if (texInitRequests.empty()) {
            if (pool.isDoingWork()) {
                std::this_thread::yield();
                continue;
            } else if (texInitRequests.empty()) {
                break;
            }
        }

        auto [idx, image] = texInitRequests.popNow();
        auto& state = itemStates[idx];

        auto texture = new CCTexture2D();
        if (!texture->initWithImage(image)) {
            delete image;
            delete texture;
            log::warn("PreloadManager: failed to init texture for '{}'", state.path);
            continue;
        }

        state.texture = texture;
        texCache->m_pTextures->setObject(texture, state.path);

        texture->release(); // bring ref count back to 1
        delete image; // no longer needed

        // cache the icon texture
        this->setCachedIcon((int) state.item.iconType, state.item.iconId, texture);

        inited++;
    }

    log::debug("PreloadManager: initialized {} textures in {}, adding frames", inited, timePreInit.elapsed().toString());
    auto timePreFrames = Instant::now();

    // TODO: bring over the pugixml sprite frame parsing code from blaze?

    for (size_t i = 0; i < itemStates.size(); i++) {
        pool.pushTask([i, &itemStates] {
            auto texCache = CCTextureCache::get();
            auto sfCache = CCSpriteFrameCache::get();
            auto& state = itemStates[i];
            auto& self = PreloadManager::get();

            if (!state.texture) {
                log::warn("PreloadManager: texture for '{}' was not initialized", state.path);
                return;
            }

            auto plistKey = fmt::format("{}.plist", state.item.image);
            auto& loaded = self.m_loadedFrames;

            if (std::find(loaded.begin(), loaded.end(), plistKey) != loaded.end()) {
                log::info("PreloadManager: already loaded frames for '{}', skipping", state.item.image);
                return;
            }

            auto pathsv = std::string_view{state.path};
            std::string fullPlistPath = std::string(pathsv.substr(0, pathsv.find(".png"))) + ".plist";

            // file reading is not thread safe on android, so we need to lock it

            CCDictionary* dict;
            {
                GEODE_ANDROID(auto _lock = g_fileMutex.lock());

                dict = CCDictionary::createWithContentsOfFileThreadSafe(fullPlistPath.c_str());
                if (!dict) {
                    log::info("PreloadManager: dict is nullptr for {}, trying slower fallback option", fullPlistPath);

                    std::string_view attemptedPlist = relativizeIconPath(fullPlistPath);

                    auto fallbackPath = self.fullPathForFilename(attemptedPlist);
                    log::info("PreloadManager: attempted fallback: {}", fallbackPath);
                    dict = CCDictionary::createWithContentsOfFileThreadSafe(fallbackPath.c_str());
                }
            }

            if (!dict) {
                log::warn("PreloadManager: failed to find the plist for '{}'", state.path);

                // remove the texture from the cache
                auto _lock = cocosLock.lock();
                texCache->m_pTextures->removeObjectForKey(state.path);
                return;
            } else {
                auto _lock = cocosLock.lock();

                // hacky!
                _addSpriteFramesWithDictionary(dict, state.texture);
                self.m_loadedFrames.push_back(plistKey);
            }

            if (dict) dict->release(); // release after unlocking
        });
    }

    // wait for all frames to be added
    pool.join();

    log::debug(
        "PreloadManager: initialized all frames in {}, total time: {}",
            timePreFrames.elapsed().toString(),
            timeBegin.elapsed().toString()
    );
}

void PreloadManager::initSessionState() {
    m_sstate.preInitTime = Instant::now();
    m_sstate.texQuality = getTextureQuality();

    fs::path resourceDir = dirs::getGameDir() / "Resources";
    fs::path textureLdrUnzipped = dirs::getGeodeDir() / "unzipped" / "geode.texture-loader" / "resources";

    size_t idx = 0;
    for (const auto& path : CCFileUtils::get()->getSearchPaths()) {
        std::string_view sv{path};
        auto fspath = fs::path(sv);

        if (sv.find("geode.texture-loader") != sv.npos) {
            // this might be the unzipped/ folder, if so, ignore it
            // if the check failed, or if the path isn't equivalent to unzipped, count it as a texture pack folder
            if (!asp::fs::equivalent(fspath, textureLdrUnzipped).unwrapOr(false)) {
                m_sstate.hasTexturePack = true;
                m_sstate.texturePackIndices.push_back(idx);
            }
        }

#ifdef GEODE_IS_ANDROID
        if (sv == "assets/") {
            m_sstate.gameSearchPathIdx = idx;
        }
#elif defined(GEODE_IS_MACOS) || defined(GEODE_IS_IOS)
        if (sv.empty()) {
            m_sstate.gameSearchPathIdx = idx;
        }
#else
        if (asp::fs::equivalent(fspath, resourceDir).unwrapOr(false)) {
            m_sstate.gameSearchPathIdx = idx;
        }
#endif

        idx++;
    }

    // spawn from 4 threads up to the cpu count
    size_t workers = std::max<size_t>(std::thread::hardware_concurrency(), 4);
    m_sstate.threadPool = std::make_unique<asp::ThreadPool>(workers);
    m_sstate.initialized = true;
    m_sstate.postInitTime = Instant::now();

    log::debug("PreloadManager: initialized session state in {}", m_sstate.postInitTime.durationSince(m_sstate.preInitTime).toString());
}

void PreloadManager::resetState() {
    m_loadQueue = {};
    m_totalCount = 0;
    m_sstate = {};
}

bool PreloadManager::shouldPreload() {
    return !m_loadQueue.empty();
}

void PreloadManager::enterContext(PreloadContext context) {
    m_context = context;

    // if we are reloading textures, everything must be reset
    if (context == PreloadContext::Reloading) {
        m_iconsLoaded = false;
        m_deathEffectsLoaded = false;
        m_loadedFrames.clear();
        m_loadedIcons.clear();
    }

    this->initLoadQueue();
}

void PreloadManager::exitContext() {
    m_context = PreloadContext::None;
    this->resetState();
}

size_t PreloadManager::getLoadedCount() {
    return m_totalCount - m_loadQueue.size();
}

size_t PreloadManager::getTotalCount() {
    return m_totalCount;
}

bool PreloadManager::deathEffectsLoaded() {
    return m_deathEffectsLoaded;
}

CCTexture2D* PreloadManager::getCachedIcon(int iconType, int id) {
    auto key = std::make_pair(iconType, id);
    auto it = m_loadedIcons.find(key);
    if (it != m_loadedIcons.end()) {
        return it->second;
    }

    return nullptr;
}

void PreloadManager::setCachedIcon(int iconType, int id, cocos2d::CCTexture2D* texture) {
    auto key = std::make_pair(iconType, id);
    m_loadedIcons[key] = texture;
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

// this function scares me
gd::string PreloadManager::fullPathForFilename(std::string_view input, bool ignoreSuffix) {
    auto& fu = HookedFileUtils::get();

    if (input.empty()) {
        return {};
    }

    // if the input is an absolute path, return it as is
    // we try to make this check as cheap as possible, so don't rely on std::filesystem or cocos
#ifdef GEODE_IS_WINDOWS
    if (input.size() >= 3 && std::isalpha(input[0]) && input[1] == ':' && (input[2] == '/' || input[2] == '\\')) {
        return gd::string{input};
    } else if (input.size() >= 2 && input[0] == '\\' && input[1] == '\\') {
        return gd::string{input};
    }
#else
    if (input.size() >= 1 && input[0] == '/') {
        return gd::string{input.data(), input.size()};
    }
#endif

    // add the quality suffix if needed
    std::string filename;

    if (!ignoreSuffix) {
        bool hasQualitySuffix = input.ends_with("-hd.png") ||
                                input.ends_with("-uhd.png") ||
                                input.ends_with("-hd.plist") ||
                                input.ends_with("-uhd.plist");

        if (!hasQualitySuffix) {
            if (input.ends_with(".plist")) {
                filename = input.substr(0, input.find(".plist"));
                appendQualitySuffix(filename, getTextureQuality(), true);
            } else {
                filename = input.substr(0, input.find(".png"));
                appendQualitySuffix(filename, getTextureQuality(), false);
            }
        }
    }

    if (filename.empty()) {
        filename = input;
    }

    // we disregard CCFileUtils m_pFilenameLookupDict / getNewFilename() here,
    // no one uses it anyway fortunately

    std::string fullpath;
    auto& searchPaths = fu.getSearchPaths();

    // we discard resolution directories here, since no one uses them
#define TRY_PATH(sp) \
    auto _fp = fu.getPathForFilename(filename, "", sp); \
    if (!_fp.empty()) { \
        return _fp; \
    }

    // check the texture pack overrides first
    for (size_t tpidx : m_sstate.texturePackIndices) {
        auto& sp = searchPaths.at(tpidx);
        TRY_PATH(sp);
    }

    // try the gd resource folder
    if (m_sstate.gameSearchPathIdx != -1) {
        auto& sp = searchPaths.at(m_sstate.gameSearchPathIdx);
        TRY_PATH(sp);
    }

    log::warn("PreloadManager: missed all known paths, trying everything: {}", input);

    // try all search paths
    for (const auto& sp : searchPaths) {
        TRY_PATH(sp);
    }

    // if all else fails, accept defeat
    log::warn("PreloadManager: could not find full path for '{}' (transformed: '{}')", input, filename);
    return filename;
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

// TODO: maybe use AAssetManager again, need to compare
unsigned char* getFileDataThreadSafe(const char* path, const char* mode, unsigned long* outSize) {
#ifndef GEODE_IS_ANDROID
    auto fu = CCFileUtils::get();
    return fu->getFileData(path, mode, outSize);
#else
    auto fu = CCFileUtils::get();
    auto _lck = g_fileMutex.lock();
    return fu->getFileData(path, mode, outSize);
#endif
}

}