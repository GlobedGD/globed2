#pragma once

#include <globed/core/data/MultiColor.hpp>

#include <asp/time/Instant.hpp>
#include <Geode/Geode.hpp>
#include <AdvancedLabel.hpp>

namespace globed {

struct Color3F {
    float r, g, b;
};

class GradientLabel : public cocos2d::CCNodeRGBA {
public:
    constexpr static size_t MAX_COLORS = 31;

    static GradientLabel* create(std::string_view text, const std::string& font);

    void limitLabelWidth(float maxw, float defaults, float mins);
    void setGradientColors(const MultiColor& color);
    void setGradientColors(const std::vector<Color3>& colors);
    void setGradientSpeed(float mod);
    void setGlobalTime(bool global);
    void setColor(const Color3& color) override;
    void setOpacity(uint8_t op) override;
    void setString(std::string_view text);

private:
    bool init(std::string_view text, const std::string& font);
    void initShader();
    void draw() override;

    Label* m_label;
    geode::Ref<cocos2d::CCGLProgram> m_shader;
    std::array<Color3F, MAX_COLORS + 1> m_colors;
    size_t m_colorCount;
    bool m_shaderEnabled;
    bool m_globalTime = true;
    float m_speedMod = 1.f;

    asp::time::Instant m_startTime;
    static const inline asp::time::Instant g_globalTimer = asp::time::Instant::now();
};

}