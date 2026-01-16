#include <globed/core/data/MultiColor.hpp>
#include <globed/util/assert.hpp>

#include <qunet/buffers/ByteReader.hpp>

using namespace geode::prelude;

#define READER_UNWRAP(...) GEODE_UNWRAP((__VA_ARGS__).mapErr([&](auto &&err) { return err.message(); }));

namespace globed {

Result<Color3> readRgb(qn::ByteReader &reader)
{
    auto r = READER_UNWRAP(reader.readU8());
    auto g = READER_UNWRAP(reader.readU8());
    auto b = READER_UNWRAP(reader.readU8());

    return Ok(Color3{r, g, b});
}

Result<std::vector<Color3>> readColorList(qn::ByteReader &reader, size_t count)
{
    std::vector<Color3> out;

    for (size_t i = 0; i < count; i++) {
        out.push_back(GEODE_UNWRAP(readRgb(reader)));
    }

    return Ok(std::move(out));
}

MultiColor::MultiColor(Type type, std::vector<Color3> colors) : m_type(type), m_colors(std::move(colors)) {}

MultiColor::MultiColor() : MultiColor(Type::Static, {Color3{255, 255, 255}}) {}

bool MultiColor::isMultiple() const
{
    return m_colors.size() > 1;
}

bool MultiColor::isGradient() const
{
    return m_type == Type::Gradient;
}

bool MultiColor::isTint() const
{
    return m_type == Type::Tinting;
}

const std::vector<Color3> &MultiColor::getColors() const
{
    return m_colors;
}

Color3 MultiColor::getColor() const
{
    GLOBED_ASSERT(m_colors.size() && "MultiColor has 0 colors");
    return m_colors[0];
}

void MultiColor::animateNode(CCRGBAProtocol *label) const
{
    constexpr int tag = 34925671;

    log::debug("re animating");

    auto node = typeinfo_cast<CCNode *>(label);

    if (!label || !node)
        return;

    node->stopActionByTag(tag);

    if (!this->isMultiple()) {
        label->setColor(m_colors[0]);
        return;
    }

    // set the last color
    label->setColor(m_colors[m_colors.size() - 1]);

    // create an action to tint between the rest of the colors
    CCArray *actions = CCArray::create();

    for (const auto &color : m_colors) {
        actions->addObject(CCTintTo::create(0.8f, color.r, color.g, color.b));
    }

    CCRepeat *action = CCRepeat::create(CCSequence::create(actions), 99999999);
    action->setTag(tag);

    node->runAction(action);
}

Result<MultiColor> MultiColor::decode(std::span<const uint8_t> data)
{
    qn::ByteReader reader{data};
    auto header = READER_UNWRAP(reader.readU8());
    size_t count = header & 0b00111111;

    Type type;
    std::vector<Color3> colors;

    switch (header >> 6) {
    case 0b00:
        return Err("Invalid color type");
    case 0b01: {
        type = Type::Static;
        colors.push_back(GEODE_UNWRAP(readRgb(reader)));
    } break;

    case 0b10: {
        type = Type::Tinting;
        colors = GEODE_UNWRAP(readColorList(reader, count));
    } break;

    case 0b11: {
        type = Type::Gradient;
        colors = GEODE_UNWRAP(readColorList(reader, count));
    } break;
    }

    return Ok(MultiColor(type, std::move(colors)));
}

MultiColor MultiColor::fromColor(const Color3 &color)
{
    return MultiColor(Type::Static, {color});
}

} // namespace globed