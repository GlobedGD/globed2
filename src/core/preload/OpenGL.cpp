#include "OpenGL.hpp"

using namespace geode::prelude;

void OpenGLInfo::initialize() {
    if (initialized) return;

    this->initOpenGLVersion();
    this->initFeatures();
    initialized = true;

    log::info("PBO support: {}, tex storage: {}, parsed version: {}.{}",
        supportsPBO ? "yes" : "no",
        supportsImmutableTex ? "yes" : "no",
        version.first, version.second
    );
    auto v = (const char*)glGetString(GL_VERSION);
    log::info("OpenGL version: {}", v ? v : "<null>");
}

void OpenGLInfo::initOpenGLVersion() {
    constexpr std::pair<int, int> DEFAULT = {2, 0};

    auto versionP = (const char*) glGetString(GL_VERSION);

    this->versionStr = versionP ? std::string_view{versionP} : std::string_view{};
    if (this->versionStr.empty()) {
        this->version = DEFAULT;
        return;
    }

#ifdef GL_MAJOR_VERSION
    glGetIntegerv(GL_MAJOR_VERSION, &version.first);
    glGetIntegerv(GL_MINOR_VERSION, &version.second);

    if (version.first != 0) {
        return;
    }
#endif
    // this is weird bullshit
    auto ver = this->versionStr;
    if (ver.starts_with("OpenGL ")) {
        ver.remove_prefix(7);
    }

    if (ver.starts_with("ES ")) {
        ver.remove_prefix(3);
    }

    // skip until the number
    auto numStart = ver.find_first_of("1234567890");
    if (numStart == ver.npos) {
        this->version = DEFAULT;
        return;
    }

    ver = ver.substr(numStart);
    auto dotPos = ver.find('.');
    if (dotPos == ver.npos || dotPos == ver.size() - 1) {
        this->version = DEFAULT;
        return;
    }

    char majorC = ver[0], minorC = ver[dotPos + 1];
    this->version = {majorC - '0', minorC - '0'};
}

bool OpenGLInfo::supportsGLExtension(std::string_view ext) {
#ifdef GL_NUM_EXTENSIONS
    // this does not exist in macos wine
    auto pglGetStringi = glGetStringi;

    if (pglGetStringi) {
        GLint exts = 0;
        glGetIntegerv(GL_NUM_EXTENSIONS, &exts);

        for (GLint i = 0; i < exts; i++) {
            auto e = reinterpret_cast<const char*>(pglGetStringi(GL_EXTENSIONS, i));
            if (e && std::string_view{e} == ext) {
                return true;
            }
        }
    }
#endif

    static auto extsStr = glGetString(GL_EXTENSIONS);
    if (!extsStr) return false;

    return asp::iter::split(std::string_view{(const char*)extsStr}, ' ')
        .any([&](std::string_view e) { return e == ext; });
}

void OpenGLInfo::initFeatures() {
#if defined(GEODE_IS_ANDROID)
    pglTexStorage2D = (globed_PFNGLTEXSTORAGE2D)eglGetProcAddress("glTexStorage2D");
    pglMapBufferRange = (globed_PFNGLMAPBUFFERRANGE)eglGetProcAddress("glMapBufferRange");
#else
    pglTexStorage2D = glTexStorage2D;
    pglMapBufferRange = glMapBufferRange;
#endif

#if defined(GEODE_IS_DESKTOP)
    // PBOs are supported on OpenGL 2.1+, but we want glMapBufferRange which is 3.0+
    this->supportsPBO = version.first >= 3;

    // Immutable textures are a core feature of OpenGL 4.2+
    this->supportsImmutableTex = version.first >= 4 && version.second >= 2;
#else
    // On mobile, PBOs and immutable textures are both GLES 3.0+
    this->supportsPBO = version.first >= 3;
    this->supportsImmutableTex = version.first >= 3;
#endif

    // check for extensions
    if (!this->supportsImmutableTex) {
        this->supportsImmutableTex = supportsGLExtension("GL_ARB_texture_storage") || supportsGLExtension("GL_EXT_texture_storage");
    }
    if (!this->supportsPBO) {
        this->supportsPBO =
            (supportsGLExtension("GL_ARB_pixel_buffer_object") || supportsGLExtension("GL_EXT_pixel_buffer_object"))
            && (supportsGLExtension("GL_EXT_map_buffer_range") || supportsGLExtension("GL_ARB_map_buffer_range"));
    }

    // don't use the functions if support is not advertised
    if (!this->supportsImmutableTex) pglTexStorage2D = nullptr;
    if (!this->supportsPBO) pglMapBufferRange = nullptr;

    // and of course, the other way around too
    if (!pglTexStorage2D) this->supportsImmutableTex = false;
    if (!pglMapBufferRange) this->supportsPBO = false;
}
