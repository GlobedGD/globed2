#include "cocos.hpp"

#include <filesystem>

#include <util/format.hpp>
#include <util/debug.hpp>

using namespace geode::prelude;

// all of this is needed to disrespect the privacy of ccfileutils
#include <Geode/modify/CCFileUtils.hpp>
class $modify(HookedCCFileUtils, CCFileUtils) {
    gd::map<gd::string, gd::string>& pubGetFullPathCache() {
        return m_fullPathCache;
    }

    gd::vector<gd::string>& pubGetSearchPathArray() {
        return m_searchPathArray;
    }

    gd::vector<gd::string>& pubGetSearchResolutionOrder() {
        return m_searchResolutionsOrderArray;
    }

    gd::string pubGetNewFilename(const char* arg) {
        return this->getNewFilename(arg);
    }

    gd::string pubGetPathForFilename(const gd::string &filename, const gd::string &resolutionDirectory, const gd::string &searchPath) {
        return this->getPathForFilename(filename, resolutionDirectory, searchPath);
    }
};

namespace util::cocos {
    // big hack

    template <typename TC>
    using priv_method_t = void(TC::*)(CCDictionary*, CCTexture2D*);

    template <typename TC, priv_method_t<TC> func>
    struct priv_caller {
        friend void privAddSpriteFramesWithDictionary(CCDictionary* p1, CCTexture2D* p2) {
            auto* obj = CCSpriteFrameCache::sharedSpriteFrameCache();
            (obj->*func)(p1, p2);
        }
    };

    template struct priv_caller<CCSpriteFrameCache, &CCSpriteFrameCache::addSpriteFramesWithDictionary>;

    void privAddSpriteFramesWithDictionary(CCDictionary* p1, CCTexture2D* p2);

    void loadAssetsParallel(const std::vector<std::string>& images) {
        const size_t MAX_THREADS = 25;
        static sync::ThreadPool threadPool(MAX_THREADS);
        // mutex for things that aren't safe to do on other threads.
        static sync::WrappingMutex<void> cocosWorkMutex;

        auto textureCache = CCTextureCache::sharedTextureCache();
        auto sfCache  = CCSpriteFrameCache::sharedSpriteFrameCache();

        struct ImageLoadState {
            std::string key;
            std::string path;
            CCImage* image = nullptr;
            CCTexture2D* texture = nullptr;
        };

        sync::WrappingMutex<std::vector<ImageLoadState>> imgStates;

        auto texNameSuffix = getTextureNameSuffix();
        auto plistNameSuffix = getTextureNameSuffixPlist();

        // this is such a hack
        size_t searchPathIdx = findSearchPathIdxForFile("icons/robot_41-hd.png");

        for (const auto& imgkey : images) {
            auto pathKey = fmt::format("{}.png", imgkey);

            // textureForKey uses slow CCFileUtils::fullPathForFilename so we reimpl it
            auto fullpath = fullPathForFilename(pathKey, searchPathIdx);

            if (fullpath.empty()) {
                continue;
            }

            std::string transformed = transformPathWithQuality(fullpath, texNameSuffix);

            if (textureCache->m_pTextures->objectForKey(transformed) != nullptr) {
                continue;
            }

            auto imgs = imgStates.lock();
            imgs->push_back(ImageLoadState {
                .key = imgkey,
                .path = transformed,
                .image = nullptr,
                .texture = nullptr
            });
        }

        auto imgCount = imgStates.lock()->size();

        for (size_t i = 0; i < imgCount; i++) {
            threadPool.pushTask([=, &imgStates] {
                auto imgState = imgStates.lock()->at(i);

                // on android, resources are read from the apk file, so it's NOT thread safe. add a lock.

#ifdef GEODE_IS_ANDROID
                auto _rguard = cocosWorkMutex.lock();
#endif

                unsigned long filesize = 0;
                unsigned char* buffer = CCFileUtils::sharedFileUtils()->getFileData(imgState.path.c_str(), "rb", &filesize);

#ifdef GEODE_IS_ANDROID
                _rguard.unlock();
#endif

                if (!buffer || filesize == 0) {
                    log::warn("failed to read file: {}", imgState.path);
                    return;
                }

                std::unique_ptr<unsigned char[]> buf(buffer);

                auto* image = new CCImage;
                if (!image->initWithImageData(buf.get(), filesize, cocos2d::CCImage::kFmtPng)) {
                    delete image;
                    log::warn("failed to init image: {}", imgState.path);
                    return;
                }

                imgStates.lock()->at(i).image = image;
            });
        }

        // wait for image adding to finish
        threadPool.join();

        // now - since textures can only be initialized on the main thread, we block for a bit to do exactly that
        auto imgsg = imgStates.lock();
        for (auto& img : *imgsg) {
            if (!img.image) continue;

            auto texture = new CCTexture2D;
            if (!texture->initWithImage(img.image)) {
                delete texture;
                img.image->release();
                img.image = nullptr;
                log::warn("failed to init texture: {}", img.path);
                continue;
            }

            img.texture = texture;
            textureCache->m_pTextures->setObject(texture, img.path);

            texture->release();
            img.image->release();
        }

        // now, add sprite frames
        imgCount = imgsg->size();
        imgsg.unlock();

        for (size_t i = 0; i < imgCount; i++) {
            threadPool.pushTask([=, &imgStates] {
                auto imgState = imgStates.lock()->at(i);
                if (!imgState.texture) return;

                // addSpriteFramesWithFile rewritten

                auto plistKey = fmt::format("{}.plist", imgState.key);

                std::string fullPlistPath;

                {
                    auto _ = cocosWorkMutex.lock();
                    fullPlistPath = fullPathForFilename(plistKey, searchPathIdx);
                    fullPlistPath = transformPathWithQualityPlist(fullPlistPath, plistNameSuffix);

                    if (sfCache->m_pLoadedFileNames->contains(plistKey)) {
                        log::debug("already contains, skipping");
                        return;
                    }
                }

                std::string texturePath;

                CCDictionary* dict = CCDictionary::createWithContentsOfFileThreadSafe(fullPlistPath.c_str());
                if (!dict) {
                    log::warn("dict is nullptr for: {}", fullPlistPath);
                    return;
                }

                CCDictionary* metadataDict = static_cast<CCDictionary*>(dict->objectForKey("metadata"));

                if (metadataDict) {
                    texturePath = metadataDict->valueForKey("textureFileName")->getCString();
                }

                if (!texturePath.empty()) {
                    // in actual cocos source, this line is not commented. but it breaks things.

                    // texturePath = CCFileUtils::sharedFileUtils()->fullPathFromRelativeFile(texturePath.c_str(), plistKey.c_str());
                    // log::debug("non empty setting to {} (from {})", texturePath, plistKey);
                } else {
                    texturePath = plistKey;

                    // remove extension, append png

                    texturePath = texturePath.erase(texturePath.find_last_of("."));
                    texturePath = texturePath.append(".png");

                    // log::debug("empty setting to {}", texturePath);
                }

                // log::debug("texpath: {}, key: {}", texturePath, imgState.key);
                {
                    auto _ = cocosWorkMutex.lock();
                    privAddSpriteFramesWithDictionary(dict, imgState.texture);
                    sfCache->m_pLoadedFileNames->insert(plistKey);
                }

                dict->release();
            });
        }

        threadPool.join();
    }

    std::string fullPathForFilename(const std::string_view filename, size_t preferIdx) {
        auto* fu = static_cast<HookedCCFileUtils*>(CCFileUtils::sharedFileUtils());

        std::string strFileName(filename);
        if (fu->isAbsolutePath(strFileName)) {
            return strFileName;
        }

        // Already Cached ?
        if (fu->pubGetFullPathCache().contains(strFileName)) {
            return fu->pubGetFullPathCache().at(strFileName);
        }

        // Get the new file name.
        std::string newFilename = fu->pubGetNewFilename(strFileName.c_str());

        std::string fullpath = "";

        // if user specified an index, try it
        if (preferIdx != -1) {
            auto& searchPath = fu->pubGetSearchPathArray().at(preferIdx);
            for (const auto& resOrder : fu->pubGetSearchResolutionOrder()) {
                std::string fullpath = fu->pubGetPathForFilename(newFilename, resOrder, searchPath);

                if (!fullpath.empty()) {
                    fu->pubGetFullPathCache().insert(std::make_pair(strFileName, fullpath));
                    return fullpath;
                }
            }
        }

        for (const auto& searchPath : fu->pubGetSearchPathArray()) {
            for (const auto& resOrder : fu->pubGetSearchResolutionOrder()) {
                std::string fullpath = fu->pubGetPathForFilename(newFilename, resOrder, searchPath);

                if (!fullpath.empty()) {
                    fu->pubGetFullPathCache().insert(std::make_pair(strFileName, fullpath));
                    return fullpath;
                }
            }
        }

        // The file wasn't found, return the file name passed in.
        return strFileName;
    }

    size_t findSearchPathIdxForFile(const std::string_view filename) {
        auto* fu = static_cast<HookedCCFileUtils*>(CCFileUtils::sharedFileUtils());
        std::string newFilename = fu->pubGetNewFilename(std::string(filename).c_str());

        size_t idx = 0;
        for (const auto& searchPath : fu->pubGetSearchPathArray()) {
            for (const auto& resOrder : fu->pubGetSearchResolutionOrder()) {
                std::string fullpath = fu->pubGetPathForFilename(newFilename, resOrder, searchPath);

                if (!fullpath.empty()) {
                    return idx;
                }
            }

            idx++;
        }

        return -1;
    }

    const char* getTextureNameSuffix() {
        std::string fp = CCFileUtils::sharedFileUtils()->fullPathForFilename("icons/robot_41.png", false);

        if (fp.ends_with("-uhd.png")) {
            return "-uhd.png";
        } else if (fp.ends_with("-hd.png")) {
            return "-hd.png";
        } else {
            return ".png";
        }
    }

    const char* getTextureNameSuffixPlist() {
        auto* tn = getTextureNameSuffix();
        if (strcmp(tn, "-uhd.png") == 0) {
            return "-uhd.plist";
        } else if (strcmp(tn, "-hd.png") == 0) {
            return "-hd.plist";
        } else {
            return ".plist";
        }
    }

    std::string transformPathWithQuality(const std::string_view path, const std::string_view suffix) {
        if (path.ends_with("-uhd.png") || path.ends_with("-hd.png")) {
            auto k = path.substr(0, path.find_last_of("-"));
            return std::string(k) + std::string(suffix);
        } else {
            return std::string(path.substr(0, path.find_last_of("."))) + std::string(suffix);
        }
    }

    std::string transformPathWithQualityPlist(const std::string_view path, const std::string_view suffix) {
        if (path.ends_with("-uhd.plist") || path.ends_with("-hd.plist")) {
            auto k = path.substr(0, path.find_last_of("-"));
            return std::string(k) + std::string(suffix);
        } else {
            return std::string(path.substr(0, path.find_last_of("."))) + std::string(suffix);
        }
    }
}