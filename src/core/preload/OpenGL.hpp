#pragma once

#include <Geode/platform/cplatform.h>
#include <utility>

#ifdef GEODE_IS_WINDOWS
#include <Geode/cocos/platform/win32/CCGL.h>

static std::pair<int, int> getOpenGLVersion() {
    int major = 0, minor = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    return {major, minor};
}

static bool supportsGLExtension(std::string_view ext) {
    GLint exts = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &exts);

    for (GLint i = 0; i < exts; i++) {
        auto e = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i));
        if (e == ext) {
            return true;
        }
    }
    return false;
}

static bool supportsPBO() {
    static bool does = [] {
        auto [major, minor] = getOpenGLVersion();
#ifdef GEODE_IS_DESKTOP
        // PBOs are supported on OpenGL 2.1+
        return (major > 2) || (major == 2 && minor >= 1);
#else
        // On mobile, it's GLES 3.0+
        return major >= 3;
#endif
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

#else

static bool supportsPBO() { return false; }

#endif
