#include "FileUtils.hpp"

#ifdef GEODE_IS_ANDROID
# include <android/asset_manager.h>
# include <android/asset_manager_jni.h>
# include <jni.h>
# include <Geode/cocos/platform/android/jni/JniHelper.h>
#endif

using namespace geode::prelude;

namespace globed {

#ifdef GEODE_IS_ANDROID
static AAssetManager* g_assetManager = nullptr;

struct AAssetDeleter {
    void operator()(AAsset* asset) const {
        if (asset) {
            AAsset_close(asset);
        }
    }
};

static std::unique_ptr<AAsset, AAssetDeleter> openAAsset(std::string_view path, int mode) {
    if (!g_assetManager) {
        log::warn("AAssetManager not initialized, openAsset({}) will fail!", path);
        return nullptr;
    }

    // strip 'assets/'
    if (path.starts_with("assets/")) {
        path.remove_prefix(7);
    }

    StringBuffer<1024> buf;
    buf.append(path);

    return std::unique_ptr<AAsset, AAssetDeleter>(AAssetManager_open(g_assetManager, buf.c_str(), mode));
}
#endif

gd::string HookedFileUtils::getPathForFilename2(std::string_view filename, std::string_view resolutionDirectory, std::string_view searchPath) {
    std::string_view file = filename;
    std::string_view filePath;

    size_t slashPos = file.find_last_of('/');
    if (slashPos != std::string::npos) {
        filePath = file.substr(0, slashPos + 1);
        file = file.substr(slashPos + 1);
    }

    geode::utils::StringBuffer<512> buf;
    buf.append("{}{}{}", searchPath, filePath, resolutionDirectory);

#ifndef __APPLE__
    buf.append(file);

    if (this->fileExists(buf.c_str())) {
        return gd::string(buf.data(), buf.size());
    }
    return gd::string{};
#else
    // Apple quirks
    geode::utils::StringBuffer<512> fileBuf;
    fileBuf.append("{}", file);
    return globed::getPathForDirAndFilenameImpl(buf.c_str(), fileBuf.c_str());
#endif
}

std::string_view relativizeIconPath(std::string_view fullpath) {
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

std::unique_ptr<unsigned char[]> getFileDataThreadSafe(const char* path, const char* mode, unsigned long* outSize) {
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
    auto asset = openAAsset(path, AASSET_MODE_UNKNOWN);
    if (!asset) {
        log::debug("AAssetManager: failed to open asset '{}'", path);
        if (outSize) *outSize = 0;
        return nullptr;
    }

    size_t size = AAsset_getLength(asset.get());
    auto buffer = std::make_unique<unsigned char[]>(size);
    size_t bytesRead = AAsset_read(asset.get(), buffer.get(), size);
    if (bytesRead != size) {
        log::debug("AAssetManager: failed to read asset '{}'", path);
        if (outSize) *outSize = 0;
        return nullptr;
    }

    if (outSize) *outSize = size;
    return buffer;
#else
    return globed::getFileDataImpl(path, outSize);
#endif
}

#ifdef GEODE_IS_ANDROID

bool initializeAAssetManager() {
    if (g_assetManager) return true;

    JniMethodInfo t;
    if (JniHelper::getStaticMethodInfo(t, "org/fmod/FMOD", "getAssetManager", "()Landroid/content/res/AssetManager;")) {
        auto r = t.env->CallStaticObjectMethod(t.classID, t.methodID);
        t.env->DeleteLocalRef(t.classID);
        g_assetManager = AAssetManager_fromJava(t.env, r);
        return g_assetManager != nullptr;
    }
    return false;
}

bool isFileExistImpl(geode::ZStringView path) {
    return openAAsset(path, AASSET_MODE_UNKNOWN) != nullptr;
}

$on_mod(Loaded) {
    GLOBED_ASSERT(initializeAAssetManager());
}

#endif

}