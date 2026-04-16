#pragma once
#include <memory>
#include <cocos2d.h>
#include <globed/util/assert.hpp>
#include <Geode/platform/cplatform.h>
#include <Geode/utils/StringBuffer.hpp>

#ifdef GEODE_IS_WINDOWS
# include <Windows.h>
#else
# include <sys/stat.h>
# include <fcntl.h>
# include <unistd.h>
#endif

namespace globed {

/// See platform/macos/objc.mm
#if defined(__APPLE__)
std::string getPathForDirAndFilenameImpl(geode::ZStringView directory, geode::ZStringView filename);
std::unique_ptr<unsigned char[]> getFileDataImpl(geode::ZStringView path, unsigned long* outSize);
bool isFileExistImpl(geode::ZStringView path);
#elif defined(GEODE_IS_ANDROID)
bool isFileExistImpl(geode::ZStringView path);
bool initializeAAssetManager();
#endif

/// thread-safe version of CCFileUtils::getFileData.
/// Invariant: path must be a full path already.
std::unique_ptr<unsigned char[]> getFileDataThreadSafe(const char* path, const char* mode, unsigned long* outSize);

// notably not actually a hook
struct HookedFileUtils : public cocos2d::CCFileUtils {
    static HookedFileUtils& get() {
        return *static_cast<HookedFileUtils*>(CCFileUtils::get());
    }

    bool fileExists(const char* path) {
#ifdef GEODE_IS_WINDOWS
        auto attrs = GetFileAttributesA(path);

        return (attrs != INVALID_FILE_ATTRIBUTES &&
                !(attrs & FILE_ATTRIBUTE_DIRECTORY));
#else
        return globed::isFileExistImpl(path);
#endif
    }

    gd::string getPathForFilename(const gd::string& filename, const gd::string& resolutionDirectory, const gd::string& searchPath) {
        return getPathForFilename2(filename, resolutionDirectory, searchPath);
    }

    gd::string getPathForFilename2(std::string_view filename, std::string_view resolutionDirectory, std::string_view searchPath);
};

std::string_view relativizeIconPath(std::string_view fullpath);

}
