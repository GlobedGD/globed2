#pragma once
#include <globed/prelude.hpp>
#include <Geode/loader/Loader.hpp>
#include <asp/thread/ThreadPool.hpp>
#include <asp/ptr/BoxedString.hpp>

namespace globed {

bool canDirectDecode();

struct PreloadItem {
    asp::BoxedString image;
    IconType iconType;
    int iconId;

    PreloadItem(asp::BoxedString image) : image(std::move(image)), iconType(IconType::DeathEffect) {}
    PreloadItem(asp::BoxedString image, IconType iconType, int iconId)
        : image(std::move(image)), iconType(iconType), iconId(iconId) {}

    PreloadItem(PreloadItem&&) noexcept = default;
    PreloadItem& operator=(PreloadItem&&) noexcept = default;
};

struct BatchPreloadState;

enum class ItemStateEnum : uint8_t {
    /// Initial state, item has been queued but nothing has been processed yet
    Initial,

    /// Image header has been parsed, and we know the dimensions of the texture.
    /// We are now ready to pre-allocate a PBO and decode the image straight into it.
    /// This state will never happen if PBOs are not supported or ImagePlus is not installed.
    HeaderParsed,
    /// Image has been fully decoded and a CCImage is available, so a texture is ready to be created.
    ImageReady,

    /// Image has either been decoded into a PBO or data has been memcpied from CCImage to the PBO.
    /// The PBO is now ready to be finalized into a CCTexture2D.
    PboReady,

    /// Item has completely finished loading and a CCTexture2D is available.
    TextureReady,

    /// Item is complete!
    Ready,

    /// Item has failed to load
    Failed,
};

struct PreloadItemState {
    PreloadItem m_item;
    gd::string m_path;
    asp::SharedPtr<BatchPreloadState> m_batchState;
    geode::Ref<cocos2d::CCImage> m_image;
    geode::Ref<cocos2d::CCTexture2D> m_texture;
    std::unique_ptr<unsigned char[]> m_rawData;
    uint32_t m_rawSize = 0;
    uint16_t m_width = 0;
    uint16_t m_height = 0;
    GLuint m_pbo = 0;
    GLuint m_tex = 0;
    std::atomic<ItemStateEnum> m_state{ItemStateEnum::Initial};

    PreloadItemState(PreloadItem item, gd::string path, asp::SharedPtr<BatchPreloadState> batchState)
        : m_item(std::move(item)), m_path(std::move(path)), m_batchState(std::move(batchState)) {}

    // so, how's life?
    PreloadItemState(PreloadItemState&& other) noexcept;

    ItemStateEnum state() const {
        return m_state.load(std::memory_order::relaxed);
    }

    bool process();
    void cleanup();
    void enqueueImageDecode();

    bool createTexture();
    void enqueuePBOCreation();
    void finalizePBO();

    void initSpriteFrames();
};

using MtTextureCallback = geode::CopyableFunction<void(PreloadItemState&)>;

struct BatchPreloadState {
    std::vector<PreloadItemState> items;
    std::atomic<size_t> itemCount{0};
    std::atomic<size_t> completedItems{0};
    MtTextureCallback callback;
    asp::ThreadPool* pool;
};

}
