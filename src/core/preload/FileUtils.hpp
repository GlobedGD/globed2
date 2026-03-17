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
#endif

namespace globed {

// mutex that guards ccfileutils file reading
static inline asp::Mutex<> g_fileMutex;

// TODO: maybe use AAssetManager again, need to compare
inline std::unique_ptr<unsigned char[]> getFileDataThreadSafe(const char* path, const char* mode, unsigned long* outSize) {
#ifdef GEODE_IS_WINDOWS
    HANDLE file = CreateFileA(
        path,
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (file == INVALID_HANDLE_VALUE) {
        if (outSize) *outSize = 0;
        return nullptr;
    }

    LARGE_INTEGER filesize;
    if (!GetFileSizeEx(file, &filesize)) {
        CloseHandle(file);
        if (outSize) *outSize = 0;
        return nullptr;
    }

    auto buffer = std::make_unique<unsigned char[]>(filesize.QuadPart);
    DWORD bytesRead;
    if (!ReadFile(file, buffer.get(), filesize.QuadPart, &bytesRead, nullptr) || bytesRead != filesize.QuadPart) {
        CloseHandle(file);
        if (outSize) *outSize = 0;
        return nullptr;
    }

    CloseHandle(file);
    if (outSize) *outSize = filesize.QuadPart;
    return buffer;

#elif defined GEODE_IS_ANDROID
    auto fu = cocos2d::CCFileUtils::get();
    auto _lck = g_fileMutex.lock();
    return std::unique_ptr<unsigned char[]>(fu->getFileData(path, mode, outSize));
#else
    auto fu = cocos2d::CCFileUtils::get();
    return std::unique_ptr<unsigned char[]>(fu->getFileData(path, mode, outSize));
#endif
}


struct HookedFileUtils : public cocos2d::CCFileUtils {
    static HookedFileUtils& get() {
        return *static_cast<HookedFileUtils*>(CCFileUtils::get());
    }

    bool fileExists(const char* path) {
#ifdef GEODE_IS_WINDOWS
        auto attrs = GetFileAttributesA(path);

        return (attrs != INVALID_FILE_ATTRIBUTES &&
                !(attrs & FILE_ATTRIBUTE_DIRECTORY));
#elif !defined(GEODE_IS_ANDROID)
        struct stat st;
        return (stat(path, &st) == 0) && S_ISREG(st.st_mode);
#else
        return CCFileUtils::get()->isFileExist(path); // it isn't that slow since it only checks the zip file and doesnt fopen
#endif
    }

    $override
    gd::string getPathForFilename(const gd::string& filename, const gd::string& resolutionDirectory, const gd::string& searchPath) {
        return getPathForFilename2(filename, resolutionDirectory, searchPath);
    }

    gd::string getPathForFilename2(std::string_view filename, std::string_view resolutionDirectory, std::string_view searchPath) {
        std::string_view file = filename;
        std::string_view filePath;

        size_t slashPos = file.find_last_of('/');
        if (slashPos != std::string::npos) {
            filePath = file.substr(0, slashPos + 1);
            file = file.substr(slashPos + 1);
        }

        geode::utils::StringBuffer<512> buf;
        buf.append("{}{}{}{}", searchPath, filePath, resolutionDirectory, file);

        if (this->fileExists(buf.c_str())) {
            return gd::string(buf.data(), buf.size());
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

}
