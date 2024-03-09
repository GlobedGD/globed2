#include "cocos.hpp"

#include <filesystem>
#ifdef GEODE_IS_WINDOWS
# include <Geode/cocos/platform/CCSAXParser.h>
#endif

#include <util/format.hpp>
#include <util/debug.hpp>
#include <hooks/game_manager.hpp>

using namespace geode::prelude;

// all of this is needed to disrespect the privacy of ccfileutils
#include <Geode/modify/CCFileUtils.hpp>
class $modify(HookedCCFileUtils, CCFileUtils) {
    static inline std::unordered_map<std::string, std::string> ourCache;

    $override
    void purgeCachedEntries() {
        CCFileUtils::purgeCachedEntries();
        ourCache.clear();
    }

    gd::map<gd::string, gd::string>& pubGetFullPathCache() {
        return m_fullPathCache;
    }

    std::unordered_map<std::string, std::string>& ourGetFullPathCache() {
        return ourCache;
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

/* I am so sorry. */

#ifdef GEODE_IS_WINDOWS
namespace {
    typedef enum
    {
        SAX_NONE = 0,
        SAX_KEY,
        SAX_DICT,
        SAX_INT,
        SAX_REAL,
        SAX_STRING,
        SAX_ARRAY
    }CCSAXState;

    typedef enum
    {
        SAX_RESULT_NONE = 0,
        SAX_RESULT_DICT,
        SAX_RESULT_ARRAY
    }CCSAXResult;

    class CCDictMaker : public cocos2d::CCSAXDelegator
    {
    public:
        CCSAXResult m_eResultType;
        cocos2d::CCArray* m_pRootArray;
        cocos2d::CCDictionary *m_pRootDict;
        cocos2d::CCDictionary *m_pCurDict;
        std::stack<cocos2d::CCDictionary*> m_tDictStack;
        std::string m_sCurKey;   ///< parsed key
        std::string m_sCurValue; // parsed value
        CCSAXState m_tState;
        cocos2d::CCArray* m_pArray;

        std::stack<cocos2d::CCArray*> m_tArrayStack;
        std::stack<CCSAXState>  m_tStateStack;

    public:
        CCDictMaker()
            : m_eResultType(SAX_RESULT_NONE),
            m_pRootArray(NULL),
            m_pRootDict(NULL),
            m_pCurDict(NULL),
            m_tState(SAX_NONE),
            m_pArray(NULL)
        {
        }

        ~CCDictMaker()
        {
        }

        cocos2d::CCDictionary* dictionaryWithContentsOfFile(const char *pFileName)
        {
            util::debug::Benchmarker bb;
            m_eResultType = SAX_RESULT_DICT;
            cocos2d::CCSAXParser parser;

            if (false == parser.init("UTF-8"))
            {
                return NULL;
            }
            parser.setDelegator(this);

            // we replace this line with our own impl that doesn't use CCFileUtils::getFileData
            // parser.parse(pFileName);

            bool ret = false;
            unsigned long size = 0;
            char* buffer = (char*)util::cocos::getFileData(pFileName, "rt", &size);
            if (buffer && size > 0) {
                ret = parser.parse(buffer, size);
            }

            CC_SAFE_DELETE_ARRAY(buffer);

            return m_pRootDict;
        }

        cocos2d::CCArray* arrayWithContentsOfFile(const char* pFileName)
        {
            m_eResultType = SAX_RESULT_ARRAY;
            cocos2d::CCSAXParser parser;

            if (false == parser.init("UTF-8"))
            {
                return NULL;
            }
            parser.setDelegator(this);

            parser.parse(pFileName);
            return m_pArray;
        }

        void startElement(void *ctx, const char *name, const char **atts)
        {
            CC_UNUSED_PARAM(ctx);
            CC_UNUSED_PARAM(atts);
            std::string sName((char*)name);
            if( sName == "dict" )
            {
                m_pCurDict = new cocos2d::CCDictionary();
                if(m_eResultType == SAX_RESULT_DICT && m_pRootDict == NULL)
                {
                    // Because it will call m_pCurDict->release() later, so retain here.
                    m_pRootDict = m_pCurDict;
                    m_pRootDict->retain();
                }
                m_tState = SAX_DICT;

                CCSAXState preState = SAX_NONE;
                if (! m_tStateStack.empty())
                {
                    preState = m_tStateStack.top();
                }

                if (SAX_ARRAY == preState)
                {
                    // add the dictionary into the array
                    m_pArray->addObject(m_pCurDict);
                }
                else if (SAX_DICT == preState)
                {
                    // add the dictionary into the pre dictionary
                    CCAssert(! m_tDictStack.empty(), "The state is wrong!");
                    cocos2d::CCDictionary* pPreDict = m_tDictStack.top();
                    pPreDict->setObject(m_pCurDict, m_sCurKey.c_str());
                }

                m_pCurDict->release();

                // record the dict state
                m_tStateStack.push(m_tState);
                m_tDictStack.push(m_pCurDict);
            }
            else if(sName == "key")
            {
                m_tState = SAX_KEY;
            }
            else if(sName == "integer")
            {
                m_tState = SAX_INT;
            }
            else if(sName == "real")
            {
                m_tState = SAX_REAL;
            }
            else if(sName == "string")
            {
                m_tState = SAX_STRING;
            }
            else if (sName == "array")
            {
                m_tState = SAX_ARRAY;
                m_pArray = new cocos2d::CCArray();
                if (m_eResultType == SAX_RESULT_ARRAY && m_pRootArray == NULL)
                {
                    m_pRootArray = m_pArray;
                    m_pRootArray->retain();
                }
                CCSAXState preState = SAX_NONE;
                if (! m_tStateStack.empty())
                {
                    preState = m_tStateStack.top();
                }

                if (preState == SAX_DICT)
                {
                    m_pCurDict->setObject(m_pArray, m_sCurKey.c_str());
                }
                else if (preState == SAX_ARRAY)
                {
                    CCAssert(! m_tArrayStack.empty(), "The state is wrong!");
                    cocos2d::CCArray* pPreArray = m_tArrayStack.top();
                    pPreArray->addObject(m_pArray);
                }
                m_pArray->release();
                // record the array state
                m_tStateStack.push(m_tState);
                m_tArrayStack.push(m_pArray);
            }
            else
            {
                m_tState = SAX_NONE;
            }
        }

        void endElement(void *ctx, const char *name)
        {
            CC_UNUSED_PARAM(ctx);
            CCSAXState curState = m_tStateStack.empty() ? SAX_DICT : m_tStateStack.top();
            std::string sName((char*)name);
            if( sName == "dict" )
            {
                m_tStateStack.pop();
                m_tDictStack.pop();
                if ( !m_tDictStack.empty())
                {
                    m_pCurDict = m_tDictStack.top();
                }
            }
            else if (sName == "array")
            {
                m_tStateStack.pop();
                m_tArrayStack.pop();
                if (! m_tArrayStack.empty())
                {
                    m_pArray = m_tArrayStack.top();
                }
            }
            else if (sName == "true")
            {
                cocos2d::CCString *str = new cocos2d::CCString("1");
                if (SAX_ARRAY == curState)
                {
                    m_pArray->addObject(str);
                }
                else if (SAX_DICT == curState)
                {
                    m_pCurDict->setObject(str, m_sCurKey.c_str());
                }
                str->release();
            }
            else if (sName == "false")
            {
                cocos2d::CCString *str = new cocos2d::CCString("0");
                if (SAX_ARRAY == curState)
                {
                    m_pArray->addObject(str);
                }
                else if (SAX_DICT == curState)
                {
                    m_pCurDict->setObject(str, m_sCurKey.c_str());
                }
                str->release();
            }
            else if (sName == "string" || sName == "integer" || sName == "real")
            {
                cocos2d::CCString* pStrValue = new cocos2d::CCString(m_sCurValue);

                if (SAX_ARRAY == curState)
                {
                    m_pArray->addObject(pStrValue);
                }
                else if (SAX_DICT == curState)
                {
                    m_pCurDict->setObject(pStrValue, m_sCurKey.c_str());
                }

                pStrValue->release();
                m_sCurValue.clear();
            }

            m_tState = SAX_NONE;
        }

        void textHandler(void *ctx, const char *ch, int len)
        {
            CC_UNUSED_PARAM(ctx);
            if (m_tState == SAX_NONE)
            {
                return;
            }

            CCSAXState curState = m_tStateStack.empty() ? SAX_DICT : m_tStateStack.top();
            cocos2d::CCString *pText = new cocos2d::CCString(std::string((char*)ch,0,len));

            switch(m_tState)
            {
            case SAX_KEY:
                m_sCurKey = pText->getCString();
                break;
            case SAX_INT:
            case SAX_REAL:
            case SAX_STRING:
                {
                    if (curState == SAX_DICT)
                    {
                        CCAssert(!m_sCurKey.empty(), "key not found : <integer/real>");
                    }

                    m_sCurValue.append(pText->getCString());
                }
                break;
            default:
                break;
            }
            pText->release();
        }
    };
}
#endif // GEODE_IS_WINDOWS

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

    // god save me from this hell

    void loadAssetsParallel(const std::vector<std::string>& images) {
        const size_t MAX_THREADS = 25;
        // static destructors on mac don't like mutexes and threads
#ifndef GEODE_IS_MACOS
        static sync::ThreadPool threadPool(MAX_THREADS);
        static sync::WrappingMutex<void> cocosWorkMutex;
#else
        sync::ThreadPool threadPool(MAX_THREADS);
        sync::WrappingMutex<void> cocosWorkMutex;
#endif

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

        // this is such a hack
        size_t searchPathIdx = findSearchPathIdxForFile("icons/player_01-uhd.png");
        bool hasTexturePack = false;

        auto* fileUtils = static_cast<HookedCCFileUtils*>(CCFileUtils::sharedFileUtils());
        auto firstPath = fileUtils->pubGetSearchPathArray().at(0);

        // if we have a texture pack, no good. just set searchPathIdx to -1
        if (std::string(firstPath).find("geode.texture-loader") != std::string::npos) {
            searchPathIdx = -1;
            hasTexturePack = true;
        }

        for (const auto& imgkey : images) {
            auto pathKey = fmt::format("{}.png", imgkey);

            // textureForKey uses slow CCFileUtils::fullPathForFilename so we reimpl it
            std::string fullpath;

            if (hasTexturePack) {
                fullpath = fullPathForFilename(pathKey, texNameSuffix, 0);
            }

            // if we have a texture pack but resolution with idx 0 failed, try searching through everything
            // if we don't have a texture pack, use the gd resources (as set earlier to searchPathIdx)
            if (fullpath.empty()) {
                fullpath = fullPathForFilename(pathKey, texNameSuffix, searchPathIdx);
            }

            if (fullpath.empty()) {
                continue;
            }

            if (textureCache->m_pTextures->objectForKey(fullpath) != nullptr) {
                continue;
            }

            auto imgs = imgStates.lock();
            imgs->push_back(ImageLoadState {
                .key = imgkey,
                .path = fullpath,
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
                unsigned char* buffer = getFileData(imgState.path.c_str(), "rb", &filesize);

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
            // this is the slow code but is essentially equivalent to the code below
            // auto imgState = imgStates.lock()->at(i);
            // auto plistKey = fmt::format("{}.plist", imgState.key);
            // auto fp = CCFileUtils::sharedFileUtils()->fullPathForFilename(plistKey.c_str(), false);
            // sfCache->addSpriteFramesWithFile(fp.c_str());

            threadPool.pushTask([=, GEODE_MACOS(&threadPool, &cocosWorkMutex,) &imgStates] {
                auto imgState = imgStates.lock()->at(i);
                if (!imgState.texture) return;

                // addSpriteFramesWithFile rewritten

                auto plistKey = fmt::format("{}.plist", imgState.key);

                {
                    auto _ = cocosWorkMutex.lock();

                    if (static_cast<HookedGameManager*>(GameManager::get())->m_fields->loadedFrames.contains(plistKey)) {
                        log::debug("already contains, skipping");
                        return;
                    }
                }

                std::string fullPlistPath = imgState.path.substr(0, imgState.path.find(".png")) + ".plist";

                std::string texturePath;
#ifdef GEODE_IS_ANDROID
                auto _ccdlock = cocosWorkMutex.lock();
#endif

#ifdef GEODE_IS_WINDOWS
                CCDictMaker tMaker;
                CCDictionary* dict = tMaker.dictionaryWithContentsOfFile(fullPlistPath.c_str());
#else
                CCDictionary* dict = CCDictionary::createWithContentsOfFileThreadSafe(fullPlistPath.c_str());
#endif

#ifdef GEODE_IS_ANDROID
                _ccdlock.unlock();
#endif
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

                    static_cast<HookedGameManager*>(GameManager::get())->m_fields->loadedFrames.insert(plistKey);
                }

                dict->release();
            });
        }

        threadPool.join();
    }

    std::string fullPathForFilename(const std::string_view filename, const std::string_view suffix, size_t preferIdx) {
        auto* fu = static_cast<HookedCCFileUtils*>(CCFileUtils::sharedFileUtils());

        std::string strFileName;
        if (filename.ends_with(".plist")) {
            strFileName = transformPathWithQualityPlist(filename, suffix);
        } else {
            strFileName = transformPathWithQuality(filename, suffix);
        }

        if (fu->isAbsolutePath(strFileName)) {
            return strFileName;
        }

        // Already Cached ?
        if (fu->ourGetFullPathCache().find(strFileName) != fu->ourGetFullPathCache().end()) {
            return fu->ourGetFullPathCache()[strFileName];
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
                    fu->ourGetFullPathCache().insert(std::make_pair(strFileName, fullpath));
                    return fullpath;
                }
            }

            // on fail return empty string
            return "";
        }

        for (const auto& searchPath : fu->pubGetSearchPathArray()) {
            for (const auto& resOrder : fu->pubGetSearchResolutionOrder()) {
                std::string fullpath = fu->pubGetPathForFilename(newFilename, resOrder, searchPath);

                if (!fullpath.empty()) {
                    fu->ourGetFullPathCache().insert(std::make_pair(strFileName, fullpath));
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

    unsigned char* getFileData(const char* fileName, const char* mode, unsigned long* size) {
        // android sucks
#ifdef GEODE_IS_ANDROID
        return CCFileUtils::sharedFileUtils()->getFileData(fileName, mode, size);
#endif

        unsigned char* buffer = nullptr;

        *size = 0;

        std::ifstream file(fileName, std::ios::binary | std::ios::ate);
        if (file.is_open()) {
            *size = static_cast<unsigned long>(file.tellg());
            file.seekg(0, std::ios::beg);

            buffer = new unsigned char[*size];
            if (!file.read(reinterpret_cast<char*>(buffer), *size)) {
                size = 0;
                delete[] buffer;
                buffer = nullptr;
            }
        }

        if (!buffer) {
            log::warn("failed to read data from a file: {}", fileName);
        }

        return buffer;
    }
}
