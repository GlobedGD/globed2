#include "Item.hpp"
#include "FileUtils.hpp"
#include "OpenGL.hpp"
#include "PreloadManager.hpp"
#include "spriteframes.hpp"
#include <prevter.imageplus/include/events.hpp>

using namespace geode::prelude;
using enum std::memory_order;

namespace globed {

static asp::SpinLock<> cocosLock;

static bool hasImagePlus() {
    static bool result = imgp::isAvailable();
    return result;
}

bool canDirectDecode() {
    return supportsPBO() && hasImagePlus();
}

PreloadItemState::PreloadItemState(PreloadItemState&& other) noexcept
    : m_item(std::move(other.m_item)),
      m_path(std::move(other.m_path)),
      m_batchState(std::move(other.m_batchState)),
      m_image(std::move(other.m_image)),
      m_texture(std::move(other.m_texture)),
      m_pbo(std::exchange(other.m_pbo, 0)),
      m_tex(std::exchange(other.m_tex, 0)),
      m_state(other.m_state.load(relaxed))
{}

bool PreloadItemState::process() {
    switch (this->state()) {
        case ItemStateEnum::Initial: {
            // Cache these bools as this must happen on main thread
            supportsImmutableTex();
            supportsPBO();

            // Initial state - load image into memory, then kick off the decoding process in a thread
            unsigned long filesize = 0;
            auto buffer = getFileDataThreadSafe(m_path.c_str(), "rb", &filesize);

            if (!buffer || filesize == 0) {
                m_state.store(ItemStateEnum::Failed, relaxed);
                log::warn("PreloadManager: could not read file '{}'", m_path);
                return false;
            }

            m_rawData = std::move(buffer);
            m_rawSize = filesize;
            this->enqueueImageDecode();
        } break;

        case ItemStateEnum::HeaderParsed: {
            // We parsed the header and obtained with and height of the image,
            // we are now ready to allocate a PBO and decode the image straight into it
            this->enqueuePBOCreation();
        } break;

        case ItemStateEnum::ImageReady: {
            // Image is now ready, we need to initialize the texture
            // This again differs by whether we support PBOs or not
            if (supportsPBO()) {
                this->enqueuePBOCreation();
            } else {
                if (!this->createTexture()) {
                    log::warn("PreloadManager: failed to create texture for '{}'", m_path);
                    m_state.store(ItemStateEnum::Failed, relaxed);
                    return false;
                }

                // texture is now ready!
                m_state.store(ItemStateEnum::TextureReady, relaxed);
                return true;
            }
        } break;

        case ItemStateEnum::PboReady: {
            // The PBO is ready and contains all image data, we just need to finalize it into a texture.
            this->finalizePBO();
            m_state.store(ItemStateEnum::TextureReady, relaxed);
            return true;
        } break;

        case ItemStateEnum::TextureReady: {
            // Texture is ready and was added to texture cache, now we need to add sprite frames
            this->initSpriteFrames();
        } break;

        case ItemStateEnum::Failed: break;
        default: std::unreachable();
    }

    return false;
}

void PreloadItemState::cleanup() {
    // here we do cleanup that must happen on main thread
    if (m_tex) {
        glDeleteTextures(1, &m_tex);
        m_tex = 0;
    }
    if (m_pbo) {
        glDeleteBuffers(1, &m_pbo);
        m_pbo = 0;
    }
}

void PreloadItemState::enqueueImageDecode() {
    auto& pool = *m_batchState->pool;

    pool.pushTask([this] {
        if (canDirectDecode()) {
            // try to parse only the image header, this may not work if imageplus is outdated
            auto res = imgp::decode::pngHeader(m_rawData.get(), m_rawSize);

            if (res) {
                auto hdr = std::move(res).unwrap();
                m_width = hdr.width;
                m_height = hdr.height;

                m_state.store(ItemStateEnum::HeaderParsed, relaxed);
                m_batchState->callback(*this);
                return;
            } else {
                log::warn("Failed to decode PNG header: {}, falling back to full decode", res.unwrapErr());
            }
        }

        m_image = Ref<CCImage>::adopt(new CCImage());
        if (!m_image->initWithImageData(m_rawData.get(), m_rawSize, cocos2d::CCImage::kFmtPng)) {
            m_image = nullptr;
            m_rawData.reset();
            m_state.store(ItemStateEnum::Failed, relaxed);
            log::warn("PreloadManager: failed to init image '{}'", m_path);
            return;
        }

        m_rawData.reset();
        m_width = m_image->m_nWidth;
        m_height = m_image->m_nHeight;
        m_state.store(ItemStateEnum::ImageReady, relaxed);
        m_batchState->callback(*this);
    });
}

bool PreloadItemState::createTexture() {
    m_texture = Ref<CCTexture2D>::adopt(new CCTexture2D());
    if (!m_texture->initWithImage(m_image)) {
        m_texture = nullptr;
        return false;
    }
    return true;
}

void PreloadItemState::enqueuePBOCreation() {
#ifdef GEODE_IS_WINDOWS
    GLOBED_DEBUG_ASSERT(!m_tex && !m_pbo);

    int64_t width = m_width;
    int64_t height = m_height;
    int64_t byteSize = width * height * 4;

    glGenTextures(1, &m_tex);
    glGenBuffers(1, &m_pbo);

    glBindTexture(GL_TEXTURE_2D, m_tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    if (supportsImmutableTex()) {
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, width, height);
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    }

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_pbo);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, byteSize, nullptr, GL_STREAM_DRAW);

    void* ptr = glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, byteSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    m_batchState->pool->pushTask([ptr, this] mutable {
        int64_t byteSize = (int64_t)m_width * (int64_t)m_height * 4;
        if (m_image) {
            std::memcpy(ptr, m_image->m_pData, byteSize);
            m_image = nullptr; // no longer needed
        } else {
            auto res = imgp::decode::pngInto(m_rawData.get(), m_rawSize, ptr, byteSize);
            m_rawData.reset();

            if (!res) {
                log::warn("PreloadManager: failed to decode image '{}' into PBO: {}", m_path, res.unwrapErr());
                m_state.store(ItemStateEnum::Failed, relaxed);

                // enqueue cleanup
                m_batchState->callback(*this);
                return;
            }

            auto bytes = res.unwrap();
            // log::debug("{}: decoded: {} png, {} decoded, {} expected", m_path, m_rawSize, bytes, byteSize);
        }

        // notify main thread
        m_state.store(ItemStateEnum::PboReady, relaxed);
        m_batchState->callback(*this);
    });
#else
    GLOBED_ASSERT(false);
#endif
}

void PreloadItemState::finalizePBO() {
    // auto now = asp::Instant::now();
#ifdef GEODE_IS_WINDOWS
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_pbo);
    glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
    glBindTexture(GL_TEXTURE_2D, m_tex);

    int64_t w = m_width, h = m_height;
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    // unbind texture & pbo, delete the pbo
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDeleteBuffers(1, &m_pbo);
    m_pbo = 0;

    m_texture = Ref<CCTexture2D>::adopt(new CCTexture2D());
    auto& texture = m_texture;
    texture->m_uName = std::exchange(m_tex, 0);
    texture->m_tContentSize = CCSize(w, h);
    texture->m_uPixelsWide = w;
    texture->m_uPixelsHigh = h;
    texture->m_ePixelFormat = kCCTexture2DPixelFormat_RGBA8888;
    texture->m_bHasPremultipliedAlpha = false;
    texture->m_bHasMipmaps = false;

    // this is  weird
    texture->m_fMaxS = 1.f;
    texture->m_fMaxT = 1.f;

    texture->setShaderProgram(CCShaderCache::sharedShaderCache()->programForKey(kCCShader_PositionTexture));
#else
    GLOBED_ASSERT(false);
#endif
}

void PreloadItemState::initSpriteFrames() {
    auto& pool = *m_batchState->pool;
    pool.pushTask([this] {
        auto texCache = CCTextureCache::get();
        auto sfCache = CCSpriteFrameCache::get();
        auto& pm = PreloadManager::get();

        if (!m_texture) {
            log::warn("PreloadManager: texture for '{}' was not initialized", m_path);
            return;
        }

        auto plistKey = fmt::format("{}.plist", m_item.image);

        if (pm.m_loadedFrames.lock()->contains(plistKey)) {
            log::info("PreloadManager: already loaded frames for '{}', skipping", m_item.image);
            return;
        }

        auto pathsv = std::string_view{m_path};
        std::string fullPlistPath = std::string(pathsv.substr(0, pathsv.find(".png"))) + ".plist";

        // read the file
        unsigned long plistSize;
        auto plistData = getFileDataThreadSafe(fullPlistPath.c_str(), "rb", &plistSize);
        if (!plistData || plistSize == 0) {
            log::info("PreloadManager: can't load {}, trying slower fallback option", fullPlistPath);
            std::string_view attemptedPlist = relativizeIconPath(fullPlistPath);
            auto fallbackPath = pm.fullPathForFilename(attemptedPlist);
            plistData = getFileDataThreadSafe(fallbackPath.c_str(), "rb", &plistSize);
        }

        if (!plistData || plistSize == 0) {
            log::warn("PreloadManager: failed to find the plist for '{}'", m_path);

            // remove the texture from the cache
            auto _lock = cocosLock.lock();
            texCache->m_pTextures->removeObjectForKey(m_path);
            return;
        }

        pm.m_loadedFrames.lock()->insert(plistKey);

        auto res = parseSpriteFrames(plistData.get(), plistSize);
        if (!res) {
            log::warn("PreloadManager: failed to parse sprite frames for '{}': {}", m_path, res.unwrapErr());
            return;
        }
        auto spf = std::move(res).unwrap();

        auto _lock = cocosLock.lock();
        addSpriteFrames(*spf, m_texture);

        m_state.store(ItemStateEnum::Ready, relaxed);
    });
}

}
