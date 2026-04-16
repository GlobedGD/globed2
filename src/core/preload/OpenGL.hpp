#pragma once

#include <Geode/platform/cplatform.h>
#include <asp/iter.hpp>
#include <utility>

# if defined(GEODE_IS_MACOS)
#  define GL_DO_NOT_WARN_IF_MULTI_GL_VERSION_HEADERS_INCLUDED
#  include <OpenGL/gl3.h>
#  include <OpenGL/gl3ext.h>
# elif defined (GEODE_IS_ANDROID)
#  include <Geode/cocos/platform/CCGL.h>
#  include <EGL/egl.h>
# elif defined (GEODE_IS_IOS)
#  include <OpenGLES/ES3/gl.h>
#  include <OpenGLES/ES3/glext.h>
# else
#  include <Geode/cocos/platform/CCGL.h>
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

static struct OpenGLInfo {
    globed_PFNGLTEXSTORAGE2D   pglTexStorage2D   = nullptr;
    globed_PFNGLMAPBUFFERRANGE pglMapBufferRange = nullptr;
    std::string_view versionStr;
    std::pair<int, int> version{0, 0};
    bool initialized = false;
    bool supportsPBO = false;
    bool supportsImmutableTex = false;

    void initialize();

private:
    void initOpenGLVersion();
    void initFeatures();
    static bool supportsGLExtension(std::string_view ext);
} g_opengl;

