#include "integrity.hpp"

#include <managers/settings.hpp>
#include <util/cocos.hpp>

#include <asp/fs.hpp>

using namespace geode::prelude;

static bool g_checked = false;
static bool g_disable = false;
static bool g_severelyBroken = false;
static bool g_fallbackMenuButton = false;
static globed::IntegrityReport g_report;
constexpr auto RESOURCE_DUMMY = "dummy-icon2.png"_spr;

static std::string pathToString(std::filesystem::path const& path) {
#ifdef GEODE_IS_WINDOWS
    return utils::string::wideToUtf8(path.wstring());
#else
    return path.string();
#endif
}

bool globed::hasBrokenResources() {
    if (!g_checked) {
        checkResources();
    }

    return g_disable;
}

bool globed::hasSeverelyBrokenResources() {
    if (!g_checked) {
        checkResources();
    }

    return g_severelyBroken;
}

bool globed::useFallbackMenuButton() {
    if (!g_checked) {
        checkResources();
    }

    return g_fallbackMenuButton;
}

void globed::resetIntegrityCheck() {
    g_checked = false;
    g_disable = false;
    g_fallbackMenuButton = false;
    g_report = {};
}

void globed::checkResources() {
    g_checked = true;

    if (GlobedSettings::get().launchArgs().skipResourceCheck) {
        // skip check entirely
        return;
    }

    g_report = std::move(createIntegrityReport());

    if (!g_report.dummmyPngFound) {
        log::warn("Failed to find {}, trying to apply builtin textures", RESOURCE_DUMMY);
        g_disable = true;
    }

    if (!g_report.menuIconPngFound) {
        log::warn("Failed to find menuicon.png, fallback menu button enabled");
        g_disable = true;
        g_fallbackMenuButton = true;
    }

    if (g_report.sheetFilesSeparated) {
        log::warn("Sheet .png and .plist files are separated, trying to apply builtin textures");
        g_disable = true;
    }

    if (g_disable) {
        // if textures are broken, try to override the sheets with ours
        auto sfc = CCSpriteFrameCache::get();
        auto tc = CCTextureCache::get();
        auto fu = CCFileUtils::get();

        auto tq = util::cocos::getTextureQuality();
        std::string_view suffix;
        switch (tq) {
            case util::cocos::TextureQuality::Medium:
                suffix = "-hd"; break;
            case util::cocos::TextureQuality::High:
                suffix = "-uhd"; break;
            default:
                break;
        }

        auto replace = [&](std::string_view what) {
            auto pngkey = fmt::format("{}.png"_spr, what);
            auto plistkey = fmt::format("{}.plist"_spr, what);
            auto plistsufkey = fmt::format("{}{}.plist"_spr, what, suffix);

            sfc->removeSpriteFramesFromFile(plistkey.c_str());
            tc->removeTextureForKey(fu->fullPathForFilename(pngkey.c_str(), false).c_str());

            auto plistpath = Mod::get()->getResourcesDir() / fmt::format("{}{}.plist", what, suffix);
            auto pngpath = Mod::get()->getResourcesDir() / fmt::format("{}{}.png", what, suffix);

            auto pngstr = pathToString(pngpath);

            fu->m_fullPathCache[fmt::format("{}.plist"_spr, what)] = pathToString(plistpath);
            fu->m_fullPathCache[fmt::format("{}.png"_spr, what)] = pngstr;
            fu->m_fullPathCache[fmt::format("{}{}.plist"_spr, what, suffix)] = pathToString(plistpath);
            fu->m_fullPathCache[fmt::format("{}{}.png"_spr, what, suffix)] = pngstr;

            tc->addImage(pngstr.c_str(), false);
            sfc->addSpriteFramesWithFile(plistsufkey.c_str());
        };

        auto iterator = asp::fs::iterdir(Mod::get()->getResourcesDir());
        if (!iterator) {
            log::warn("Failed to iterate over globed resource folder: {}", iterator.unwrapErr().message());
            g_severelyBroken = true;
            return;
        }

        for (auto file : iterator.unwrap()) {
            auto filename = pathToString(file.path().filename());
            if (filename.ends_with(".plist") && !filename.ends_with("-hd.plist") && !filename.ends_with("-uhd.plist")) {
                log::warn("Replacing sheet with original textures: '{}'", filename);
                replace(filename.substr(0, filename.size() - 6));
            }
        }

        // check if this actually helped

        bool menuIcon = util::cocos::isValidSprite(CCSprite::createWithSpriteFrameName("menuicon.png"_spr));
        bool dummyPng = util::cocos::isValidSprite(CCSprite::createWithSpriteFrameName(RESOURCE_DUMMY));

        if (!menuIcon || !dummyPng) {
            log::warn("Failed to restore Globed textures, mod will be disabled.");
            g_severelyBroken = true;
        } else {
            log::info("Textures restored");
        }

        g_fallbackMenuButton = !menuIcon;
    }
}

globed::IntegrityReport& globed::getIntegrityReport() {
    return g_report;
}

std::string globed::IntegrityReport::asDebugData() {
    // Debug data format:
    // gs1:0 - all is good
    // gs1:-1 - globedsheet1.png not found
    // gs1:-2 - globedsheet1.plist not found
    // gs1:1 - tp detected, png and plist are in different folders
    // gs1:2 - tp detected, png or plist in wrong folder
    // dmv4:1 - darkmode v4 detected
    // impf:0 - impostor folder not found
    // impf:1 - impostor folder found
    // rss:0 - all resources seem fine
    // rss:1 - resource dummy is missing
    // rss:2 - menuicon.png is missing
    // plat:x - platform (a32 for Android32, a64 for Android64, w for Windows, ma for ARM Mac, mi for Intel Mac)
    // ver:x - version of the mod

    std::string data;

    if (sheetIntegrity == SheetIntegrity::MissingPng) {
        data += "gs1:-1;";
    } else if (sheetIntegrity == SheetIntegrity::MissingPlist) {
        data += "gs1:-2;";
    } else if (sheetIntegrity == SheetIntegrity::Ok) {
        if (sheetFilesSeparated) {
            data += "gs1:1;";
        } else if (sheetFilesModified) {
            data += "gs1:2;";
        } else {
            data += "gs1:0;";
        }
    }

    if (darkmodeEnabled) {
        data += "dmv4:1;";
    }

    if (impostorFolder) {
        data += "impf:1;";
    } else {
        data += "impf:0;";
    }

    if (!menuIconPngFound) {
        data += "rss:2;";
    } else if (!dummmyPngFound) {
        data += "rss:1;";
    } else {
        data += "rss:0;";
    }

    // add platform and version to debug data
    data += fmt::format("plat:{};ver:{};",
        GEODE_ANDROID32("a32") GEODE_ANDROID64("a64") GEODE_WINDOWS("w") GEODE_ARM_MAC("ma") GEODE_INTEL_MAC("mi") GEODE_IOS("i"),
        Mod::get()->getVersion()
    );

    return data;
}

globed::IntegrityReport globed::createIntegrityReport() {
    IntegrityReport report{};

    // First, check for texture packs
    // check if filename of globedsheet1.png is overriden
    auto p = CCFileUtils::get()->fullPathForFilename("globedsheet1.png"_spr, false);
    auto plist = CCFileUtils::get()->fullPathForFilename("globedsheet1.plist"_spr, false);

    auto pngParent = std::filesystem::path(std::string(p)).parent_path();
    auto plistParent = std::filesystem::path(std::string(plist)).parent_path();

    if (p.empty()) {
        log::error("Failed to find globedsheet1.png (critical!)");
        report.sheetIntegrity = SheetIntegrity::MissingPng;
    } else if (plist.empty()) {
        log::error("Failed to find globedsheet1.plist (critical!)");
        report.sheetIntegrity = SheetIntegrity::MissingPlist;
    } else if (!asp::fs::equivalent(pngParent, plistParent).unwrapOr(false)) {
        log::warn("Mismatch, globedsheet1.plist and globedsheet1.png are in different places ({} and {})", p, plist);
        report.sheetIntegrity = SheetIntegrity::Ok;
        report.sheetFilesSeparated = true;
    } else if (!asp::fs::equivalent(pngParent, Mod::get()->getResourcesDir())) {
        log::warn("Mismatch, globedsheet1 expected in {}, found at {}", Mod::get()->getResourcesDir(), p);
        report.sheetIntegrity = SheetIntegrity::Ok;
        report.sheetFilesModified = true;
    }

    report.hasTexturePack = report.sheetIntegrity != SheetIntegrity::Ok || report.sheetFilesSeparated || report.sheetFilesModified;
    log::debug("Texture pack detected: {}", report.hasTexturePack);

    // check for darkmode mod
    report.darkmodeEnabled = Loader::get()->isModLoaded("bitz.darkmode_v4");
    log::debug("DarkMode v4 enabled: {}", report.darkmodeEnabled);

    // check for impostor folder

    // TODO: for android, check if the dankmeme.globed2 folder is in the apk

    auto impostorFolderLoc = dirs::getGameDir() / "Resources" / "dankmeme.globed2";

    if (asp::fs::exists(impostorFolderLoc)) {
        report.impostorFolder = impostorFolderLoc;
        log::warn("Impostor folder found: {}", impostorFolderLoc.string());
    }

    // check for menuicon.png and dummy-icon1.png
    report.menuIconPngFound = util::cocos::isValidSprite(CCSprite::createWithSpriteFrameName("menuicon.png"_spr));
    report.dummmyPngFound = util::cocos::isValidSprite(CCSprite::createWithSpriteFrameName(RESOURCE_DUMMY));

    log::debug("Resource check, menu icon: {}, dummy icon: {}", report.menuIconPngFound, report.dummmyPngFound);

    return report;
}

const std::filesystem::path& globed::getLatestLogFile() {
    return log::getCurrentLogPath();
}