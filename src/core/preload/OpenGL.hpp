#pragma once

#include <Geode/platform/cplatform.h>
#include <utility>

#ifndef GEODE_IS_IOS
# define GLOBED_PBO_SUPPORT
#endif

#ifdef GLOBED_PBO_SUPPORT
# include <Geode/cocos/platform/CCGL.h>
# if defined (GEODE_IS_MOBILE)
#  include <EGL/egl.h>
# endif

# ifndef GL_PIXEL_UNPACK_BUFFER
#  define GL_PIXEL_UNPACK_BUFFER 0x88EC
# endif
# ifndef GL_MAP_INVALIDATE_BUFFER_BIT
#  define GL_MAP_INVALIDATE_BUFFER_BIT 0x0008
# endif
# ifndef GL_MAP_WRITE_BIT
#  define GL_MAP_WRITE_BIT 0x0002
# endif
# ifndef GL_RGBA8
#  define GL_RGBA8 0x8058
# endif

using globed_PFNGLTEXSTORAGE2D = void(*)(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
using globed_PFNGLMAPBUFFERRANGE = void*(*)(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access);

static globed_PFNGLTEXSTORAGE2D   pglTexStorage2D   = nullptr;
static globed_PFNGLMAPBUFFERRANGE pglMapBufferRange = nullptr;

static std::pair<int, int> getOpenGLVersion() {
#ifdef GL_MAJOR_VERSION
    int major = 0, minor = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    return {major, minor};
#else
    auto v = glGetString(GL_VERSION);
    if (!v) return {2, 0};

    std::string_view ver{(const char*) v};
    if (ver.contains("OpenGL ES 3")) return {3, 0};

    return {2, 0};
#endif
}

static bool supportsGLExtension(std::string_view ext) {
#ifdef GL_NUM_EXTENSIONS
    GLint exts = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &exts);

    for (GLint i = 0; i < exts; i++) {
        auto e = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i));
        if (e == ext) {
            return true;
        }
    }
#else
    auto exts = glGetString(GL_EXTENSIONS);
    return std::string_view{(const char*)exts}.contains(ext);
#endif
    return false;
}

static bool supportsPBO() {
    static bool does = [] {
        auto [major, minor] = getOpenGLVersion();
#ifdef GEODE_IS_DESKTOP
        // PBOs are supported on OpenGL 2.1+
        if ((major > 2) || (major == 2 && minor >= 1)) {
            return true;
        }
#else
        // On mobile, it's GLES 3.0+
        if (major >= 3) {
            return true;
        }
#endif
        return supportsGLExtension("GL_EXT_pixel_buffer_object");
    }();
    return does;
}

static bool supportsImmutableTex() {
    static bool does = [] {
        auto [major, minor] = getOpenGLVersion();
#ifdef GEODE_IS_DESKTOP
        // Core feature of OpenGL 4.2+
        if ((major > 4) || (major == 4 && minor >= 2)) {
            return true;
        }
#else
        // On mobile, it's GLES 3.0+
        if (major >= 3) {
            return true;
        }
#endif

        return supportsGLExtension("GL_ARB_texture_storage") || supportsGLExtension("GL_EXT_texture_storage");
    }();
    return does;
}


static void initGL() {
    supportsPBO();
    supportsImmutableTex();

#ifdef GEODE_IS_MOBILE
    pglTexStorage2D = (globed_PFNGLTEXSTORAGE2D)eglGetProcAddress("glTexStorage2D");
    pglMapBufferRange = (globed_PFNGLMAPBUFFERRANGE)eglGetProcAddress("glMapBufferRange");
#else
    pglTexStorage2D = glTexStorage2D;
    pglMapBufferRange = glMapBufferRange;
#endif
}


#else

static bool supportsPBO() { return false; }
static bool supportsImmutableTex() { return false; }

#endif // GLOBED_PBO_SUPPORT
