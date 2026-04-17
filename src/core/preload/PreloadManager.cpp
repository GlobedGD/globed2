#include "PreloadManager.hpp"
#include <globed/prelude.hpp>
#include <globed/core/SettingsManager.hpp>
#include <globed/util/FunctionQueue.hpp>
#include <asp/fs.hpp>
#include <asp/sync/SpinLock.hpp>
#include <asp/format.hpp>
#include "FileUtils.hpp"
#include "Item.hpp"

#include <Geode/modify/CCTextureCache.hpp>
#include <Geode/modify/CCSpriteFrameCache.hpp>

#ifdef _WIN32
# include <Windows.h>
#elif defined __APPLE__
# include <mach/mach.h>
# include <unistd.h>
#else
# include <sys/sysinfo.h>
#endif

using namespace asp::time;
using namespace geode::prelude;
namespace fs = std::filesystem;

namespace globed {

static std::optional<PreloadItem> makeIconItem(IconType type, int id) {
    auto gm = globed::singleton<GameManager>();
    auto sheetName = gm->sheetNameForIcon(id, (int)type);
    if (sheetName.empty()) {
        return std::nullopt;
    }

    return PreloadItem{
        asp::BoxedString{std::string_view{sheetName}},
        type, id
    };
}

static PreloadItem makeDeathEffectItem(int id) {
    return PreloadItem {
        asp::BoxedString::format("PlayerExplosion_{:02}.png", id - 1)
    };
}

PreloadManager::PreloadManager() {
    // spawn ncpus+4 threads, since some amount of time is spent on blocking (mutexes, file io)
    size_t workers = std::thread::hardware_concurrency() + 4;
    m_threadPool = std::make_unique<asp::ThreadPool>(workers);
}

PreloadManager::~PreloadManager() {}

void PreloadManager::initLoadQueue() {
    auto memoryMb = this->getAvailableMemory() / 1024 / 1024;
    bool enoughForIcons = memoryMb && memoryMb > 500;
    bool enoughForEffects = memoryMb && memoryMb > 900;
    log::info("PreloadManager: available system memory: {} MB", memoryMb);

    auto gm = globed::singleton<GameManager>();
    m_totalCount = 0;
    m_loadQueue.clear();

    if (!globed::setting<bool>("core.preload.enabled") || Mod::get()->getSettingValue<bool>("force-no-preload")) {
        // preloading is disabled, skip everything
        return;
    }

    if (globed::setting<bool>("core.preload.defer")) {
        // preloading is deferred, only load if we are loading a level
        if (m_context != PreloadContext::Level) {
            return;
        }
    }

    if (!enoughForIcons) {
        log::warn("PreloadManager: skipping all preloading due to low system memory ({} MB free)", memoryMb);
        return;
    }

    // Death effects
    // skip if death effects are disabled in settings / default death effects are enabled / low on ram
    bool loadDe = globed::setting<bool>("core.player.death-effects") && !globed::setting<bool>("core.player.default-death-effects");
    if (loadDe && !enoughForEffects) {
        log::warn("PreloadManager: skipping death effects due to low system memory ({} MB free)", memoryMb);
        loadDe = false;
    }

    if (!m_deathEffectsLoaded && loadDe) {
        m_deathEffectsLoaded = true;

        for (size_t i = 2; i < 21; i++) {
            m_loadQueue.emplace_back(makeDeathEffectItem(i));
        }
    }

    if (!m_iconsLoaded) {
        m_iconsLoaded = true;

        auto addIcons = [&](IconType ty, int startIdx, int endIdx) {
            for (int id = startIdx; id <= endIdx; id++) {
                if (auto item = makeIconItem(ty, id)) {
                    m_loadQueue.emplace_back(std::move(*item));
                }
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

        // for (size_t i = 1; i <= 7; i++ ){
        //     m_loadQueue.emplace_back(asp::BoxedString::format("streak_{:02}_001.png", i));
        //     m_loadQueue.back().iconType = IconType::Special;
        // }
    }

    m_totalCount = m_loadQueue.size();
}

void PreloadManager::loadNextBatch(PreloadOptions options) {
    if (!m_sstate.initialized) {
        this->initSessionState();
    }

    static size_t limit = [&] {
        // random heuristic to decide how many textures to put in one batch to not freeze for too long
        // smaller batches = more responsive loading UI
        // larger batches = gets work done much faster (less waiting)
        float deviceFactor = 1.f
            GEODE_WINDOWS(* 2.5f)
            GEODE_MACOS(* 2.f)
            GEODE_ANDROID(* 1.25f);

        size_t threads = std::thread::hardware_concurrency();
        float threadFactor = std::powf(std::max<size_t>(threads, 2), 0.8f);
        float qualityFactor = 1.f;

        switch (m_sstate.texQuality) {
            case TextureQuality::Low: qualityFactor = 2.5f; break;
            case TextureQuality::Medium: qualityFactor = 1.5f; break;
            case TextureQuality::High: qualityFactor = 1.f; break;
        }

        size_t base = 250;

        return (float)base * deviceFactor * threadFactor * qualityFactor;

        // here are some example outputs of this computation:
        // * Android 8-core CPU, medium quality -> 791 items
        // * iOS 4-core CPU, high quality -> 303 items
        // * Windows 4-core CPU, low quality -> 1894 items
        // * Windows 8-core CPU, high quality - 1319 items
    }();

    size_t setLimit = globed::setting<int>("core.preload.batch-size");
    if (setLimit > 0) {
        limit = setLimit;
    }

    size_t count = 0;

    std::vector<PreloadItem> items;

    while (count < limit && !m_loadQueue.empty()) {
        items.emplace_back(std::move(m_loadQueue.front()));
        m_loadQueue.pop_front();

        auto& item = items.back();
        if (item.iconType == IconType::DeathEffect) {
            count += 20;
        } else {
            count++;
        }
    }

    int remainingCost = asp::iter::from(m_loadQueue)
        .map([](auto item) {
            return item.get().iconType == IconType::DeathEffect ? 20 : 1;
        })
        .sum();

    // if remaining cost is too small, just load everything to avoid a tiny batch
    if (remainingCost > 0 && remainingCost < 100) {
        items.insert(
            items.end(),
            std::make_move_iterator(m_loadQueue.begin()),
            std::make_move_iterator(m_loadQueue.end())
        );
        m_loadQueue.clear();
    }

    log::trace("PreloadManager: loadNextBatch: loading {} items", items.size());

    this->doLoadBatch(std::move(items), std::move(options));
}

void PreloadManager::loadEverything(PreloadOptions options) {
    std::vector<PreloadItem> items;
    items.reserve(m_loadQueue.size());

    items.insert(items.end(), std::make_move_iterator(m_loadQueue.begin()), std::make_move_iterator(m_loadQueue.end()));
    m_loadQueue.clear();

    log::trace("PreloadManager: loadEverything: loading {} items", items.size());

    this->doLoadBatch(std::move(items), std::move(options));
}

void PreloadManager::loadIcons(PlayerIconData icons, PreloadOptions options) {
    std::vector<PreloadItem> items;

    auto doPush = [&](IconType ty, int id) {
        if (auto item = makeIconItem(ty, id)) {
            items.emplace_back(std::move(*item));
        }
    };

    doPush(IconType::Cube, icons.cube);
    doPush(IconType::Ship, icons.ship);
    doPush(IconType::Ball, icons.ball);
    doPush(IconType::Ufo, icons.ufo);
    doPush(IconType::Wave, icons.wave);
    doPush(IconType::Robot, icons.robot);
    doPush(IconType::Spider, icons.spider);
    doPush(IconType::Swing, icons.swing);
    doPush(IconType::Jetpack, icons.jetpack);

    bool loadDeath = !m_deathEffectsLoaded
        && icons.deathEffect > 1
        && !asp::iter::contains(m_loadingDeathEffects, icons.deathEffect)
        && !asp::iter::contains(m_loadedDeathEffects, icons.deathEffect);

    if (loadDeath) {
        items.emplace_back(makeDeathEffectItem(icons.deathEffect));
        m_loadingDeathEffects.push_back(icons.deathEffect);

        // cheat a little
        options.completionCallback = [this, id = icons.deathEffect, cb = std::move(options.completionCallback)] mutable {
            log::trace("finished loading death effect {}", id);

            m_loadedDeathEffects.push_back(id);
            auto it = std::ranges::find(m_loadingDeathEffects, id);
            if (it != m_loadingDeathEffects.end()) {
                m_loadingDeathEffects.erase(it);
            }

            if (cb) cb();
        };
    }

    this->doLoadBatch(std::move(items), std::move(options));
}

void PreloadManager::loadIcon(IconType ty, int icon, PreloadOptions options) {
    std::vector<PreloadItem> items;

    if (auto item = makeIconItem(ty, icon)) {
        items.emplace_back(std::move(*item));
    }

    this->doLoadBatch(std::move(items), std::move(options));
}

void PreloadManager::doLoadBatch(std::vector<PreloadItem> items, PreloadOptions options) {
    if (!m_sstate.initialized) {
        this->initSessionState();
    }

    auto timeBegin = Instant::now();

    auto texCache = CCTextureCache::get();
    auto sfCache = CCSpriteFrameCache::get();
    auto fu = CCFileUtils::get();
    auto& pool = *m_threadPool;

    auto state = asp::make_shared<BatchPreloadState>();
    state->pool = &pool;
    state->m_blockingMode = options.blocking;
    state->completionCallback = std::move(options.completionCallback);
    state->items.reserve(items.size());

    if (options.blocking) {
        // if blocking, push into the channel to be handled later
        state->callback = [](auto& state, auto& tex) {
            state.texRequests.push(&tex);
        };
    } else {
        // otherwise offload stuff to qimt
        state->callback = [](auto& state, auto& tex) {
            state.texRequests.push(&tex);
            state.maybeEnqueueMTUpdate();
        };
    }

    // Stage 1 - enqueue all items for loading
    for (auto& item : items) {
        StringBuffer<512> buf;

        if (item.image.ends_with(".png")) {
            buf.append("{}", item.image);
        } else {
            buf.append("{}.png", item.image);
        }

        auto fullPath = this->fullPathForFilename(buf.view());
        if (fullPath.empty()) {
            log::warn("PreloadManager: could not find full path for '{}'", item.image);
            continue;
        }

        if (texCache->m_pTextures->objectForKey(fullPath)) {
            // texture already loaded, skip
            // log::trace("PreloadManager: texture already loaded for '{}', skipping", fullPath);
            continue;
        }

        auto& itemState = state->items.emplace_back(std::move(item), std::move(fullPath), state);

        state->callback(*state, itemState);
        state->itemCount.fetch_add(1, std::memory_order::relaxed);
    }

    log::trace("PreloadManager: {} items were enqueued", state->itemCount.load(std::memory_order::relaxed));

    // if non-blocking, that's all we need to do
    if (!options.blocking) {
        // if we have not queued any textures, call the callback anyway to finish
        if (state->itemCount.load(std::memory_order::relaxed) == 0) {
            state->maybeEnqueueMTUpdate();
        }
        return;
    }

    auto drawFrame = [&] {
        auto dir = CCDirector::get();
        dir->m_bPaused = true;
        dir->drawScene();
        dir->m_bPaused = false;
    };

    auto lastProgress = asp::Instant::now();
    size_t nextProgressWhenTex = 128;
    auto tryDispatchProgress = [&](size_t completed) {
        if (options.blocking && options.callback) {
            size_t itemCount = state->itemCount.load(std::memory_order::relaxed);
            PreloadProgress prog{
                // getLoadedCount essentially returns total - remaining, we need to subtract uninited items from this batch
                .totalLoaded = this->getLoadedCount() - (itemCount - completed),
                .totalCount = m_totalCount,
                .batchLoaded = completed,
                .batchSize = itemCount,
            };
            log::debug("PreloadManager: dispatching batch progress: {} textures, {} complete, {} in total", state->initedTextures.load(std::memory_order::relaxed), completed, itemCount);

            if (options.callback(prog)) {
                drawFrame();
            }

            lastProgress = Instant::now();
            nextProgressWhenTex = completed + 128;
        }
    };

    auto timePreStage2 = Instant::now();
    std::optional<Instant> timePostTextures;

    // Stage 2 - wait and process all requests. Main thread will be responsible for advancing the state machine forward,
    // as well as creating PBOs or initializing textures manually.
    while (true) {
        size_t completed = state->completedItems.load(std::memory_order::relaxed);
        if (completed >= nextProgressWhenTex) {
            tryDispatchProgress(completed);
        }

        state->doProcess();

        if (pool.isDoingWork()) {
            // draw frames manually every once in a while, so the game doesn't appear frozen
            if (lastProgress.elapsed() > Duration::fromMillis(50)) {
                tryDispatchProgress(state->completedItems.load(std::memory_order::relaxed));
            }
            std::this_thread::yield();
            continue;
        } else if (state->hasFinished()) {
            break;
        }
    }

    auto timeEnd = Instant::now();
    size_t totalItems = state->itemCount.load(std::memory_order::relaxed);
    size_t completeItems = state->completedItems.load(std::memory_order::relaxed);

    if (!timePostTextures) {
        timePostTextures = timePreStage2;
    }

    log::debug(
        "PreloadManager: batch of {}/{} complete in {} ({} prepare, {} tex, {} frames)",
        completeItems, totalItems,
        timeEnd.durationSince(timeBegin),
        timePreStage2.durationSince(timeBegin),
        timePostTextures->durationSince(timePreStage2),
        timeEnd.durationSince(*timePostTextures)
    );

    // free everything, since we have a ref cycle in items
    state->cleanup();
}

void PreloadManager::initSessionState() {
    m_sstate.preInitTime = Instant::now();
    m_sstate.texQuality = getTextureQuality();

    fs::path resourceDir = dirs::getGameDir() / "Resources";
    fs::path textureLdrUnzipped = dirs::getGeodeDir() / "unzipped" / "geode.texture-loader" / "resources";
    fs::path highGraphicsFolder;

    if (auto mod = Loader::get()->getLoadedMod("weebify.high-graphics-android")) {
        highGraphicsFolder = mod->getConfigDir(false) / Loader::get()->getGameVersion();
        if (!asp::fs::exists(highGraphicsFolder)) {
            highGraphicsFolder.clear();
        }
    }

    size_t idx = 0;
    for (const auto& path : CCFileUtils::get()->getSearchPaths()) {
        std::string_view sv{path};
        auto fspath = fs::path(sv);

        if (asp::fs::equivalent(fspath, highGraphicsFolder).unwrapOr(false)) {
            m_sstate.highTexturesIdx = idx;
        }

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

    m_sstate.initialized = true;
    m_sstate.postInitTime = Instant::now();

    log::info("PreloadManager: initialized session state in {}", m_sstate.postInitTime.durationSince(m_sstate.preInitTime).toString());
    log::debug(
        "TPs: {}, quality: {}, game idx: {}, high graphics idx: {}, tp idxs: {}",
        m_sstate.hasTexturePack, (int)m_sstate.texQuality,
        m_sstate.gameSearchPathIdx, m_sstate.highTexturesIdx,
        m_sstate.texturePackIndices
    );
}

void PreloadManager::resetState() {
    m_loadQueue.clear();
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
        log::info("PreloadManager: resetting state due to texture reload");
        m_iconsLoaded = false;
        m_deathEffectsLoaded = false;
        m_loadedFrames.lock()->clear();
        m_loadedIcons.clear();
    }

    if (context == PreloadContext::Loading || context == PreloadContext::Reloading) {
        // reset session state when reloading
        this->resetState();
    }

    this->initLoadQueue();
}

void PreloadManager::exitContext() {
    m_context = PreloadContext::None;
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

bool PreloadManager::deathEffectLoaded(int id) {
    return m_deathEffectsLoaded || asp::iter::contains(m_loadedDeathEffects, id);
}

bool PreloadManager::iconsLoaded() {
    return m_iconsLoaded;
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
    // log::debug("Added {} {} to cache", iconType, id);
    m_loadedIcons[key] = texture;
}

// returns the quality suffix for the given quality, e.g. "", "-hd", "-uhd"
static std::string_view getQualitySuffix(TextureQuality quality) {
    switch (quality) {
        case TextureQuality::Low: {
            return "";
        } break;
        case TextureQuality::Medium: {
            return "-hd";
        } break;
        case TextureQuality::High: {
            return "-uhd";
        } break;
    }
}

static uint64_t fnv1aHash(std::string_view s) {
    uint64_t hash = 0xcbf29ce484222325;
    for (char c : s) {
        hash ^= static_cast<uint64_t>(c);
        hash *= 0x100000001b3;
    }
    return hash;
}

template <size_t N>
static void appendToBuf(std::array<char, N>& buf, size_t& offset, std::string_view str) {
    size_t toCopy = std::min(str.size(), N - offset - 1);
    std::memcpy(buf.data() + offset, str.data(), toCopy);
    offset += toCopy;
    buf[offset] = '\0';
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
    std::array<char, 1024> filenameBuf;
    filenameBuf[0] = '\0';
    size_t filenameOffset = 0;

    auto append = [&](std::string_view str) {
        appendToBuf(filenameBuf, filenameOffset, str);
    };

    if (!ignoreSuffix) {
        auto period = input.find_last_of('.');
        std::string_view base = input.substr(0, period);

        bool hasQualitySuffix = base.ends_with("-uhd") || base.ends_with("-hd");

        if (!hasQualitySuffix) {
            append(base);
            append(getQualitySuffix(getTextureQuality()));

            if (period != std::string::npos) {
                std::string_view extension = input.substr(period);
                append(extension);
            }
        }
    }

    if (filenameOffset == 0) {
        append(input);
    }

    // we disregard CCFileUtils m_pFilenameLookupDict / getNewFilename() here,
    // no one uses it anyway fortunately

    std::string_view filename{filenameBuf.data(), filenameOffset};
    auto& searchPaths = fu.getSearchPaths();

    // we discard resolution directories here, since no one uses them
#define TRY_PATH(sp) \
    auto _fp = fu.getPathForFilename2(filename, "", sp); \
    if (!_fp.empty()) { \
        return _fp; \
    }

    // check the texture pack overrides first
    for (size_t tpidx : m_sstate.texturePackIndices) {
        auto& sp = searchPaths.at(tpidx);
        TRY_PATH(sp);
    }

    // then check high graphics for android
    if (m_sstate.highTexturesIdx != (size_t)-1) {
        auto& sp = searchPaths.at(m_sstate.highTexturesIdx);
        TRY_PATH(sp);
    }

    // now try the gd resource folder
    if (m_sstate.gameSearchPathIdx != (size_t)-1) {
        auto& sp = searchPaths.at(m_sstate.gameSearchPathIdx);
        TRY_PATH(sp);
    }

    // try the real gd resource folder
#ifdef GEODE_IS_DESKTOP
    auto realResources = string::pathToString(dirs::getResourcesDir());
    if (!realResources.empty()) {
        TRY_PATH(realResources);
    }
#endif

    log::warn("PreloadManager: missed all known paths, trying everything: {}", input);

    // try all search paths
    for (const auto& sp : searchPaths) {
        TRY_PATH(sp);
    }

    // if all else fails, accept defeat
    log::warn("PreloadManager: could not find full path for '{}' (transformed: '{}')", input, filename);
    if (!ignoreSuffix) {
        log::warn("PreloadManager: will try again without suffix");
        return this->fullPathForFilename(input, true);
    }
    return {};
}

uint64_t PreloadManager::getAvailableMemory() {
#ifdef _WIN32
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    return static_cast<uint64_t>(memInfo.ullAvailPhys);

#elif defined __APPLE__
    mach_port_t host_port = mach_host_self();
    vm_statistics64_data_t vm_stats;
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    uint64_t pageSize = sysconf(_SC_PAGESIZE);

    if (host_statistics64(host_port, HOST_VM_INFO64, (host_info64_t)&vm_stats, &count) == KERN_SUCCESS) {
        return static_cast<uint64_t>(vm_stats.free_count + vm_stats.external_page_count) * pageSize;
    }
#else
    std::ifstream meminfo("/proc/meminfo");
    std::string linestr;
    while (std::getline(meminfo, linestr)) {
        std::string_view line{linestr};
        if (line.starts_with("MemAvailable:")) {
            line.remove_prefix(14);
            line = line.substr(line.find_first_not_of(" \t"));
            line = line.substr(0, line.find_first_of(" \t"));
            return utils::numFromString<uint64_t>(line).unwrapOr(0) * 1024;
        }
    }
#endif
    return 0;
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

}

// class $modify(CCTextureCache) {
//     CCTexture2D* addImage(const char* path, bool p) {
//         auto time = Instant::now();
//         auto res = CCTextureCache::addImage(path, p);
//         auto taken = time.elapsed();
//         if (taken.micros() > 100) {
//             log::debug("addImage('{}') took {}", path, taken);
//             auto pl = PlayLayer::get();
//             std::string_view p{path};
//             if (p.contains("icons/") && pl && pl->m_player2 && !p.ends_with("01-uhd.png") && !p.ends_with("00-uhd.png") && !p.ends_with("00.png") && !p.ends_with("01.png")) {
//                 __debugbreak();
//             }
//         }

//         return res;
//     }
// };

// class $modify(CCSpriteFrameCache) {
//     void addSpriteFramesWithFile(const char* path) {
//         auto time = Instant::now();
//         CCSpriteFrameCache::addSpriteFramesWithFile(path);
//         auto taken = time.elapsed();
//         if (taken.micros() > 100) {
//             log::debug("sprite frames('{}') took {}", path, taken);
//             auto pl = PlayLayer::get();
//             std::string_view p{path};
//             if (p.contains("icons/") && pl && pl->m_player2 && !p.ends_with("01-uhd.plist") && !p.ends_with("00-uhd.plist") && !p.ends_with("00.plist") && !p.ends_with("01.plist")) {
//                 __debugbreak();
//             }
//         }
//     }
// };
