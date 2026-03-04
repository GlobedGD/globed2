#include "Item.hpp"
#include "FileUtils.hpp"
#include "OpenGL.hpp"
#include "PreloadManager.hpp"
#include "spriteframes.hpp"
#include <prevter.imageplus/include/events.hpp>
#include <bit>

#ifdef GEODE_IS_WINDOWS
# include <immintrin.h>
#endif

using namespace geode::prelude;
using enum std::memory_order;


namespace globed {

struct ScratchBuffer {
    std::unique_ptr<uint8_t[]> m_data;
    size_t m_cap = 0;

    uint8_t* reserve(size_t required) {
        if (required == 0) return nullptr;

        if (m_cap < required) {
            m_cap = std::bit_ceil(required); // next power of 2
            m_data = std::make_unique<uint8_t[]>(m_cap);
        }
        return m_data.get();
    }
};

static asp::SpinLock<> cocosLock;
static thread_local ScratchBuffer g_scratch;

static bool shouldUsePBO() {
    static bool should = supportsPBO() && globed::setting<bool>("core.preload.use-pbos");
    return should;
}

bool canDirectDecode() {
    static bool can = shouldUsePBO() && imgp::isAvailable() && globed::setting<bool>("core.preload.use-direct-decode");
    return can;
}

static void initialize() {
#ifdef GLOBED_PBO_SUPPORT
    static bool inited = false;
    if (inited) return;
    inited = true;

    initGL();

    log::info("Using PBOs: {}, immutable textures: {}, direct decode: {}", shouldUsePBO(), supportsImmutableTex(), canDirectDecode());
    auto v = (const char*)glGetString(GL_VERSION);
    log::info("OpenGL version: {}", v ? v : "<null>");
#endif
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
            initialize();

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
            if (shouldUsePBO()) {
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
#ifdef GLOBED_PBO_SUPPORT
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
#endif

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

static void checkGL(std::string_view where) {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        log::error("GL error at {}: 0x{:X}", where, err);
    }
}

static void clearGLError() {
    while (glGetError() != GL_NO_ERROR);
}

static void premultiplyInto(const void* source, void* dest, size_t bytes);

void PreloadItemState::enqueuePBOCreation() {
#ifdef GLOBED_PBO_SUPPORT
    GLOBED_DEBUG_ASSERT(!m_tex && !m_pbo);

    int64_t width = m_width;
    int64_t height = m_height;
    int64_t byteSize = width * height * 4;

    // some cocos code leaves an error for us
    clearGLError();

    glGenTextures(1, &m_tex);
    glGenBuffers(1, &m_pbo);

    glBindTexture(GL_TEXTURE_2D, m_tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    if (supportsImmutableTex() && pglTexStorage2D) {
        pglTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, width, height);
        checkGL("glTexStorage2D");
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        checkGL("glTexImage2D");
    }

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_pbo);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, byteSize, nullptr, GL_STREAM_DRAW);

    void* ptr = pglMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, byteSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
    checkGL("glMapBufferRange");

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    m_batchState->pool->pushTask([ptr, this] mutable {
        int64_t byteSize = (int64_t)m_width * (int64_t)m_height * 4;
        if (m_image) {
            std::memcpy(ptr, m_image->m_pData, byteSize);
            m_image = nullptr; // no longer needed
        } else {
#ifdef GLOBED_PBO_SUPPORT
            // decode the image into a scratch buffer
            auto sbuf = g_scratch.reserve(byteSize);
            auto res = imgp::decode::pngInto(m_rawData.get(), m_rawSize, sbuf, byteSize);
            m_rawData.reset();

            if (!res) {
                log::warn("PreloadManager: failed to decode image '{}' into PBO: {}", m_path, res.unwrapErr());
                m_state.store(ItemStateEnum::Failed, relaxed);

                // enqueue cleanup
                m_batchState->callback(*this);
                return;
            }

            auto bytes = res.unwrap();
            GLOBED_DEBUG_ASSERT(bytes == (size_t)byteSize);
            // log::debug("{}: decoded: {} png, {} decoded, {} expected", m_path, m_rawSize, bytes, byteSize);

            // now premultiply alpha and write into the pbo
            premultiplyInto(sbuf, ptr, byteSize);
#else
            GLOBED_ASSERT(false);
#endif
        }

        // notify main thread
        m_state.store(ItemStateEnum::PboReady, relaxed);
        m_batchState->callback(*this);
    });
#else
    GLOBED_ASSERT(false);
#endif // GLOBED_PBO_SUPPORT
}

void PreloadItemState::finalizePBO() {
    // auto now = asp::Instant::now();
#ifdef GLOBED_PBO_SUPPORT
    clearGLError();
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_pbo);
    GLboolean ok = glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
    GLOBED_ASSERT(ok);
    glBindTexture(GL_TEXTURE_2D, m_tex);

    int64_t w = m_width, h = m_height;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    checkGL("glTexSubImage2D");

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
    texture->m_bHasPremultipliedAlpha = true;
    texture->m_bHasMipmaps = false;

    // this is  weird
    texture->m_fMaxS = 1.f;
    texture->m_fMaxT = 1.f;

    texture->setShaderProgram(CCShaderCache::sharedShaderCache()->programForKey(kCCShader_PositionTexture));
#else // GLOBED_PBO_SUPPORT
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
        m_batchState->completedItems.fetch_add(1, relaxed);
    });
}

static void premultiplyIntoScalar(const void* source, void* dest, size_t bytes) {
    size_t pixels = bytes / 4;
    auto src = static_cast<const uint8_t*>(source);
    auto dst = static_cast<uint8_t*>(dest);

    for (size_t i = 0; i < pixels; i++) {
        uint8_t r = src[0];
        uint8_t g = src[1];
        uint8_t b = src[2];
        uint8_t a = src[3];

        dst[0] = (r * a) / 255;
        dst[1] = (g * a) / 255;
        dst[2] = (b * a) / 255;
        dst[3] = a;

        src += 4;
        dst += 4;
    }
}

#ifdef GEODE_IS_WINDOWS
static __attribute__((target("ssse3"))) void premultiplyIntoSSSE3(const void* source, void* dest, size_t bytes) {
    size_t const max_simd_pixel = bytes / sizeof(__m128i) * sizeof(__m128i);

    __m128i const mask_alphha_color_odd_255 = _mm_set1_epi32(static_cast<int>(0xff000000));
    __m128i const div_255 = _mm_set1_epi16(static_cast<short>(0x8081));

    __m128i const mask_shuffle_alpha = _mm_set_epi32(0x0f800f80, 0x0b800b80, 0x07800780, 0x03800380);
    __m128i const mask_shuffle_color_odd = _mm_set_epi32(static_cast<int>(0x80800d80), static_cast<int>(0x80800980), static_cast<int>(0x80800580), static_cast<int>(0x80800180));

    const __m128i* src = reinterpret_cast<const __m128i*>(source);
    __m128i* dst = reinterpret_cast<__m128i*>(dest);
    __m128i color, alpha, color_even, color_odd;

    for (size_t i = 0; i < max_simd_pixel; i += sizeof(__m128i)) {
        color = _mm_loadu_si128(src);

        alpha = _mm_shuffle_epi8(color, mask_shuffle_alpha);

        color_even = _mm_slli_epi16(color, 8);
        color_odd = _mm_shuffle_epi8(color, mask_shuffle_color_odd);
        color_odd = _mm_or_si128(color_odd, mask_alphha_color_odd_255);
//            color_odd = _mm_blendv_epi8(color, _mm_set_epi32(0xff000000, 0xff000000, 0xff000000, 0xff000000), _mm_set_epi32(0x80800080, 0x80800080, 0x80800080, 0x80800080));

        color_odd = _mm_mulhi_epu16(color_odd, alpha);
        color_even = _mm_mulhi_epu16(color_even, alpha);

        color_odd = _mm_srli_epi16(_mm_mulhi_epu16(color_odd, div_255), 7);
        color_even = _mm_srli_epi16(_mm_mulhi_epu16(color_even, div_255), 7);

        color = _mm_or_si128(color_even, _mm_slli_epi16(color_odd, 8));

        _mm_storeu_si128(dst, color);

        src++;
        dst++;
    }

    size_t remBytes = bytes - max_simd_pixel;
    premultiplyIntoScalar(
        static_cast<const uint8_t*>(source) + max_simd_pixel,
        static_cast<uint8_t*>(dest) + max_simd_pixel,
        remBytes
    );
}
#endif

void premultiplyInto(const void* source, void* dest, size_t bytes) {
#ifdef GEODE_IS_WINDOWS
    // assume that any modern pc supports ssse3, it's around 3x faster than scalar
    premultiplyIntoSSSE3(source, dest, bytes);
#else
    premultiplyIntoScalar(source, dest, bytes);
#endif
}

}
