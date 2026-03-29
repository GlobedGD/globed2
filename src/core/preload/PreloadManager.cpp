#include "PreloadManager.hpp"
#include <globed/prelude.hpp>
#include <globed/core/SettingsManager.hpp>
#include <globed/util/FunctionQueue.hpp>
#include <asp/fs.hpp>
#include <asp/sync/SpinLock.hpp>
#include <asp/format.hpp>
#include "FileUtils.hpp"
#include "Item.hpp"

using namespace asp::time;
using namespace geode::prelude;
namespace fs = std::filesystem;

namespace globed {

PreloadManager::PreloadManager() {}

PreloadManager::~PreloadManager() {}

void PreloadManager::initLoadQueue() {
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

    // Death effects
    // skip if death effects are disabled in settings or default death effects are enabled
    bool loadDe = globed::setting<bool>("core.player.death-effects") && !globed::setting<bool>("core.player.default-death-effects");
    if (!m_deathEffectsLoaded && loadDe) {
        m_deathEffectsLoaded = true;

        for (size_t i = 1; i < 20; i++) {
            m_loadQueue.emplace_back(asp::BoxedString::format("PlayerExplosion_{:02}.png", i));
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

                m_loadQueue.emplace_back(
                    asp::BoxedString{std::string_view{sheetName}},
                    ty, id
                );
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


    log::debug("Hi test 3 {}", m_loadQueue.size());;
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

    this->doLoadBatch(std::move(items), std::move(options));
}

void PreloadManager::loadEverything(PreloadOptions options) {
    std::vector<PreloadItem> items;
    items.reserve(m_loadQueue.size());

    items.insert(items.end(), std::make_move_iterator(m_loadQueue.begin()), std::make_move_iterator(m_loadQueue.end()));
    m_loadQueue.clear();

    this->doLoadBatch(std::move(items), std::move(options));
}

void PreloadManager::doLoadBatch(std::vector<PreloadItem> items, PreloadOptions options) {
    if (!m_sstate.initialized) {
        this->initSessionState();
    }

    auto timeBegin = Instant::now();

    log::debug("PreloadManager: loading batch of {} items", items.size());

    auto texCache = CCTextureCache::get();
    auto sfCache = CCSpriteFrameCache::get();
    auto fu = CCFileUtils::get();
    auto& pool = *m_sstate.threadPool;

    asp::Channel<PreloadItemState*> texRequests;
    auto state = asp::make_shared<BatchPreloadState>();
    state->pool = &pool;
    state->items.reserve(items.size());

    // this guards every insertion into `state->items`. we don't wrap the entire vector into a mutex,
    // since actual accesses are safe because only one thread owns a specific item at any given time and there can be no reallocs.
    static asp::SpinLock<> itemsLock;

    if (options.blocking) {
        // if blocking, push into the channel to be handled later
        state->callback = [&texRequests](auto& tex) {
            texRequests.push(&tex);
        };
    } else {
        utils::terminate("non-blocking preload is not yet implemented");
        // // otherwise offload stuff to qimt
        // onImageLoaded = [](size_t idx, CCImage* image) {
        //     FunctionQueue::get().queue([=] {

        //     });
        // };
    }

    // Stage 1 - enqueue all items for loading, in parallel
    for (auto& item : items) {
        pool.pushTask([&item, state, texCache, this] {
            StringBuffer<512> buf;

            if (item.image.ends_with(".png")) {
                buf.append("{}", item.image);
            } else {
                buf.append("{}.png", item.image);
            }

            auto fullPath = this->fullPathForFilename(buf.view());
            if (fullPath.empty()) {
                log::warn("PreloadManager: could not find full path for '{}'", item.image);
                return;
            }

            if (texCache->m_pTextures->objectForKey(fullPath)) {
                // texture already loaded, skip
                return;
            }

            auto _lock = itemsLock.lock();
            auto& itemState = state->items.emplace_back(std::move(item), std::move(fullPath), state);
            _lock.unlock();

            state->callback(itemState);
            state->itemCount.fetch_add(1, std::memory_order::relaxed);
        });
    }

    size_t initedTextures = 0;
    auto insertTexture = [&](auto& state) {
        texCache->m_pTextures->setObject(state.m_texture, state.m_path);

        // cache the icon texture
        this->setCachedIcon((int) state.m_item.iconType, state.m_item.iconId, state.m_texture);
        initedTextures++;
    };

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
            log::debug("PreloadManager: dispatching batch progress: {} textures, {} complete, {} in total", initedTextures, completed, itemCount);

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
        if (!timePostTextures && initedTextures >= state->itemCount.load(std::memory_order::relaxed)) {
            timePostTextures = Instant::now();
        }

        if (auto req = texRequests.tryPop()) {
            auto& item = **req;

            // if the item is not ready/failed, immediately advance the state machine forward
            auto state = item.state();
            if (state != ItemStateEnum::TextureReady && state != ItemStateEnum::Failed && state != ItemStateEnum::Ready) {
                if (!item.process()) {
                    // not yet ready, result will be posted to main thread later
                    continue;
                }

                // item state has changed now!
                state = item.state();
            }

            switch (state) {
                // failure state - perform cleanup and skip
                case ItemStateEnum::Failed: {
                    item.cleanup();
                } break;

                // textureready state - the texture is complete, enqueue sprite frames task which is the final one
                case ItemStateEnum::TextureReady: {
                    insertTexture(item);
                    item.process();
                } break;

                // this should not be reached in practice, process() must only return true if the state is ready or failed
                default: GLOBED_ASSERT(false);
            }

            continue;
        }

        if (pool.isDoingWork()) {
            // draw frames manually every once in a while, so the game doesn't appear frozen
            if (lastProgress.elapsed() > Duration::fromMillis(50)) {
                tryDispatchProgress(state->completedItems.load(std::memory_order::relaxed));
            }
            std::this_thread::yield();
            continue;
        } else if (texRequests.empty()) {
            // verify that everything is truly done
            for (auto& item : state->items) {
                auto st = item.state();
                if (st != ItemStateEnum::Ready && st != ItemStateEnum::Failed) {
                    log::debug("PreloadManager: looping again, item {} is incomplete (state {})", item.m_path, (int)st);
                    std::this_thread::yield();
                    continue;
                }
            }

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
    if (options.blocking) {
        state->items.clear();
    }
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

    // spawn ncpus+4 threads, since some amount of time is spent on blocking (mutexes, file io)
    size_t workers = std::thread::hardware_concurrency() + 4;
    m_sstate.threadPool = std::make_unique<asp::ThreadPool>(workers);
    m_sstate.initialized = true;
    m_sstate.postInitTime = Instant::now();

    log::debug("PreloadManager: initialized session state in {}", m_sstate.postInitTime.durationSince(m_sstate.preInitTime).toString());
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

    // try the gd resource folder
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