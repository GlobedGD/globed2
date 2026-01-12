#include "GradientLabel.hpp"

#include <UIBuilder.hpp>

using namespace geode::prelude;
using namespace asp::time;

constexpr auto vertex = R"(
attribute vec4 a_position;
attribute vec2 a_texCoord;
attribute vec4 a_color;

#ifdef GL_ES
varying lowp vec4 v_fragmentColor;
varying mediump vec2 v_texCoord;
varying mediump vec2 v_texCoordRaw;
#else
varying vec4 v_fragmentColor;
varying vec2 v_texCoord;
varying vec2 v_texCoordRaw;
#endif

uniform vec2 uSize;

void main() {
    gl_Position = CC_MVPMatrix * a_position;
    v_fragmentColor = a_color;
    v_texCoordRaw = a_texCoord;
    v_texCoord = vec2(
        (a_position.x - 0.0) / uSize.x,
        (a_position.y - 0.0) / uSize.y
    );
}
)";

constexpr auto multiFrag = R"(
#ifdef GL_ES
precision mediump float;
#endif

varying vec4 v_fragmentColor;
varying vec2 v_texCoord;
varying vec2 v_texCoordRaw;
uniform int colorCount;
uniform vec3 colors[32];
uniform bool enabled;
uniform float customTime;
uniform sampler2D CC_Texture0;

void main() {
    if (!enabled) {
        gl_FragColor = v_fragmentColor * texture2D(CC_Texture0, v_texCoordRaw);
        return;
    }

    // keep t in [0,1)
    float t = fract(v_texCoord.x + customTime * 0.2); // customTime/5.0 == customTime*0.2

    // default color
    vec3 col = colors[0];

    if (colorCount > 1) {
        float segments = float(colorCount - 1);
        float scaledT  = t * segments;
        int   idx      = int(floor(scaledT));    // interval index in [0, segments-1]
        float localT   = scaledT - float(idx);   // position inside interval

        col = mix(colors[idx], colors[idx + 1], localT);
    }

    gl_FragColor = vec4(col, 1.0) * v_fragmentColor * texture2D(CC_Texture0, v_texCoordRaw);
}
)";

namespace globed {

static CCGLProgram* getShader() {
    auto ccsc = CCShaderCache::sharedShaderCache();
    auto program = ccsc->programForKey("gradient-label-shader"_spr);

    if (program) return program;

    auto vsh = vertex;
    auto fsh = multiFrag;

    program = new CCGLProgram();
    program->initWithVertexShaderByteArray(vsh, fsh);
    program->addAttribute("a_position", kCCVertexAttrib_Position);
    program->addAttribute("a_texCoord", kCCVertexAttrib_TexCoords);
    program->addAttribute("a_color", kCCVertexAttrib_Color);

    if (!program->link()) {
        log::error("Shader link failed!");
        program->release();
        return nullptr;
    }

    program->updateUniforms();
    ccsc->addProgram(program, "gradient-label-shader"_spr);
    program->release();

    return program;
}

bool GradientLabel::init(std::string_view text, const std::string& font) {
    CCNode::init();

    m_label = Build<Label>::create(text, font)
        .anchorPoint(0.f, 0.f)
        .parent(this);

    m_startTime = Instant::now();

    this->setCascadeColorEnabled(false);
    this->setCascadeOpacityEnabled(false);
    this->setContentSize(m_label->getContentSize());
    this->initShader();

    return true;
}

void GradientLabel::initShader() {
    m_shader = getShader();
    m_shaderEnabled = false;

    if (m_shader) {
        m_label->getChildrenExt()[0]->setShaderProgram(m_shader);
    }
}

void GradientLabel::limitLabelWidth(float maxw, float defaults, float mins) {
    m_label->limitLabelWidth(maxw, defaults, mins);
    this->setContentSize(m_label->getContentSize());
}

void GradientLabel::setColor(const Color3& color) {
    CCNodeRGBA::setColor(color);
    m_label->setColor(_displayedColor);
    m_shaderEnabled = false;
}

void GradientLabel::setOpacity(uint8_t op) {
    CCNodeRGBA::setOpacity(op);
    m_label->setOpacity(_displayedOpacity);
}

void GradientLabel::setString(std::string_view text) {
    m_label->setString(text);
    this->setContentSize(m_label->getContentSize());
}

void GradientLabel::setGradientColors(const MultiColor& color) {
    this->setGradientColors(color.getColors());
}

void GradientLabel::setGradientColors(const std::vector<Color3>& inp) {
    if (!m_shader || inp.empty()) return;

    // reset to white to properly show the gradient
    this->setColor({255, 255, 255});

    m_colorCount = std::min(inp.size() + 1, MAX_COLORS + 1);

    size_t idx = 0;
    for (auto& col : inp) {
        m_colors[idx] = Color3F {
            (float)col.r / 255.f,
            (float)col.g / 255.f,
            (float)col.b / 255.f,
        };

        idx++;
        if (idx == MAX_COLORS) break;
    }

    // push the first color as the last color, for a smooth transition!!
    m_colors[idx] = m_colors[0];
    m_shaderEnabled = true;
}

void GradientLabel::setGradientSpeed(float mod) {
    m_speedMod = mod;
}

void GradientLabel::setGlobalTime(bool global) {
    m_globalTime = global;
}

void GradientLabel::draw() {
    if (!m_shader) {
        CCNode::draw();
        return;
    }

    m_shader->use();
    m_shader->setUniformsForBuiltins();

    CCSize size = m_label->getContentSize();
    GLint sizeLoc = m_shader->getUniformLocationForName("uSize");
    m_shader->setUniformLocationWith2f(sizeLoc, size.width, size.height);

    GLint enabledLoc = m_shader->getUniformLocationForName("enabled");
    m_shader->setUniformLocationWith1i(enabledLoc, (int)m_shaderEnabled);

    GLint colorCountLoc = m_shader->getUniformLocationForName("colorCount");
    m_shader->setUniformLocationWith1i(colorCountLoc, m_colorCount);

    GLint colorsLoc = m_shader->getUniformLocationForName("colors");
    m_shader->setUniformLocationWith3fv(colorsLoc, (GLfloat*)m_colors.data(), m_colors.size());

    float time = (m_globalTime ? g_globalTimer : m_startTime).elapsed().seconds<float>();
    GLint timeLoc = m_shader->getUniformLocationForName("customTime");
    m_shader->setUniformLocationWith1f(timeLoc, time * m_speedMod);

    CCNode::draw();
}

GradientLabel* GradientLabel::create(std::string_view text, const std::string& font) {
    auto ret = new GradientLabel;
    if (ret->init(text, font)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

}