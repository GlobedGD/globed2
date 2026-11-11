#include "PreloadManager.hpp"
#include <globed/prelude.hpp>
#include <globed/core/SettingsManager.hpp>
#include <globed/util/FunctionQueue.hpp>
#include <asp/fs.hpp>
#include <asp/iter.hpp>
#include <asp/sync/SpinLock.hpp>
#include <asp/format.hpp>

#include <AsyncLoad/FileUtils.hpp>

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

namespace globed {

static std::optional<PreloadItem> makeIconItem(IconType type, int id) {
    auto gm = globed::singleton<GameManager>();
    auto sheetName = gm->sheetNameForIcon(id, (int)type);
    if (sheetName.empty()) {
        return std::nullopt;
    }

    if (sheetName.ends_with(".png")) {
        sheetName.erase(sheetName.size() - 4);
    }

    return PreloadItem{
        asp::BoxedString{std::string_view{sheetName}},
        type, id
    };
}

static PreloadItem makeDeathEffectItem(int id) {
    return PreloadItem {
        asp::BoxedString::format("PlayerExplosion_{:02}", id - 1), IconType::DeathEffect, id
    };
}

static std::optional<PreloadItem> makeItem(IconType type, int id) {
    if (type == IconType::DeathEffect) {
        return makeDeathEffectItem(id);
    } else {
        return makeIconItem(type, id);
    }
}

GLOBED_EXPORT_SINGLETON(PreloadManager, SingletonBase<PreloadManager>);

PreloadManager::PreloadManager() {}

PreloadManager::~PreloadManager() {}

AsyncLoad::TaskGroup PreloadManager::initLoadQueue() {
    auto memoryMb = this->getAvailableMemory() / 1024 / 1024;
    uint64_t mbForIcons = 300;
    uint64_t mbForEffects = 500;
    if (globed::setting<bool>("core.preload.force-preload")) {
        mbForIcons = 0;
        mbForEffects = 0;
    } else if (getTextureQuality() == TextureQuality::High) {
        mbForIcons *= 2;
        mbForEffects *= 2;
    }

    bool enoughForIcons = memoryMb >= mbForIcons;
    bool enoughForEffects = memoryMb >= mbForEffects;
    log::info("PreloadManager: available system memory: {} MB, load icons: {}, effects: {}", memoryMb, enoughForIcons, enoughForEffects);

    auto gm = globed::singleton<GameManager>();
    AsyncLoad::TaskGroup group;

    if (!globed::setting<bool>("core.preload.enabled") || Mod::get()->getSettingValue<bool>("force-no-preload")) {
        // preloading is disabled, skip everything
        return group;
    }

    if (globed::setting<bool>("core.preload.defer")) {
        // preloading is deferred, only load if we are loading a level
        if (m_context != PreloadContext::Level) {
            return group;
        }
    }

    if (!enoughForIcons) {
        log::warn("PreloadManager: skipping all preloading due to low system memory ({} MB free)", memoryMb);
        return group;
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
            this->submitItem(group, makeDeathEffectItem(i));
        }
    }

    if (!m_iconsLoaded) {
        m_iconsLoaded = true;

        auto addIcons = [&](IconType ty, int startIdx, int endIdx) {
            for (int id = startIdx; id <= endIdx; id++) {
                if (auto item = makeIconItem(ty, id)) {
                    this->submitItem(group, std::move(*item));
                }
            }
        };

        // TODO 2.209: all these ranges are 2.2081 !!!

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

    group.close([](auto) {});

    return group;
}

void PreloadManager::submitItem(AsyncLoad::TaskGroup& group, PreloadItem item, Callback callback) {
    auto& alm = AsyncLoad::ALManager::get();

    group.add(alm.loadSpritesheet(item.image, [this, cb = std::move(callback), iconType = item.iconType, iconId = item.iconId](auto result) mutable {
        auto res = this->onSheetLoaded(std::move(result), iconType, iconId);
        if (cb) cb(res);
    }, AsyncLoad::SpritesheetMergeBehavior::Never));
}

AsyncLoad::TaskHandle PreloadManager::submitItemIndependent(PreloadItem item, Callback callback) {
    auto& alm = AsyncLoad::ALManager::get();
    return alm.loadSpritesheet(item.image, [this, cb = std::move(callback), iconType = item.iconType, iconId = item.iconId](auto result) mutable {
        cb(this->onSheetLoaded(std::move(result), iconType, iconId));
    }, AsyncLoad::SpritesheetMergeBehavior::Never);
}

Result<> PreloadManager::onSheetLoaded(Result<AsyncLoad::LoadedSpritesheet> result, IconType iconType, int iconId) {
    if (result) {
        auto sheet = std::move(result).unwrap();

        if (iconType == IconType::DeathEffect) {
            log::trace("finished loading death effect {}", iconId);
            m_loadedDeathEffects.insert(iconId);
        }

        Ref<CCTexture2D> tex = std::move(sheet.texture);

        if (sheet.skipped) {
            // skipped means the texture was already loaded, retrieve it from texture cache
            if (auto item = makeItem(iconType, iconId)) {
                tex = AsyncLoad::getTexture(item->image);
            }
        }

        if (tex) {
            this->setCachedIcon((int) iconType, iconId, std::move(tex));
        } else {
            log::error("PreloadManager: texture unavailable for {} {}", (int)iconType, iconId);
        }

        return Ok();
    } else {
        log::error("Failed to load spritesheet: {}", result.unwrapErr());
        return Err(result.unwrapErr());
    }
}

void PreloadManager::loadIcons(PlayerIconData icons, Callback callback) {
    AsyncLoad::TaskGroup group;
    group.setFailBehavior(AsyncLoad::TaskGroupFailBehavior::Late);

    auto doPush = [&](IconType ty, int id) {
        if (auto item = makeIconItem(ty, id)) {
            group.add(this->submitItemIndependent(std::move(*item)));
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

    bool loadDeath = !m_deathEffectsLoaded && icons.deathEffect > 1;
    if (loadDeath) {
        group.add(this->submitItemIndependent(makeDeathEffectItem(icons.deathEffect)));
    }

    group.close([cb = std::move(callback)](AsyncLoad::TaskGroupResult tgresult) mutable {
        for (auto& r : tgresult.results) {
            if (!r.result) {
                log::warn("PreloadManager: subtask cancelled!");
            } else if (r.result->isErr()) {
                log::warn("PreloadManager: subtask failed: {}", r.result->unwrapErr());
            }
        }

        if (tgresult.status == AsyncLoad::GroupStatus::Completed) {
            if (cb) cb(Ok());
        } else {
            if (cb) cb(Err("PreloadManager: one or more subtasks failed"));
        }
    });
}

void PreloadManager::loadIcon(IconType ty, int icon, Callback callback) {
    GLOBED_ASSERT(ty != IconType::DeathEffect);

    if (auto item = makeIconItem(ty, icon)) {
        this->submitItemIndependent(std::move(*item), std::move(callback));
    } else {
        if (callback) callback(Err("invalid icon type or id"));
    }
}

AsyncLoad::TaskGroup PreloadManager::enterContext(PreloadContext context) {
    m_context = context;

    // if we are reloading textures, everything must be reset
    if (context == PreloadContext::Reloading) {
        log::info("PreloadManager: resetting state due to texture reload");
        this->releaseTextures();
    }

    return this->initLoadQueue();
}

void PreloadManager::exitContext() {
    m_context = PreloadContext::None;
}

bool PreloadManager::deathEffectsLoaded() {
    return m_deathEffectsLoaded;
}

bool PreloadManager::deathEffectLoaded(int id) {
    return m_deathEffectsLoaded || m_loadedDeathEffects.contains(id);
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

void PreloadManager::setCachedIcon(int iconType, int id, Ref<CCTexture2D> texture) {
    auto key = std::make_pair(iconType, id);
    // log::debug("Added {} {} to cache", iconType, id);
    m_loadedIcons[key] = std::move(texture);
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

void PreloadManager::releaseTextures() {
    m_loadedIcons.clear();
    m_loadedFrames.lock()->clear();
    m_iconsLoaded = false;
    m_deathEffectsLoaded = false;
    m_loadedDeathEffects.clear();
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
