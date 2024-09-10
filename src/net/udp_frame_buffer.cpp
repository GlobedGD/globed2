#include "udp_frame_buffer.hpp"

#include <data/bytebuffer.hpp>

Result<std::vector<uint8_t>> UdpFrameBuffer::pushFrameFromBuffer(ByteBuffer& buf) {
    uint32_t packetId;
    uint8_t frameIdx, frameCount;

    auto rres = [&]() -> ByteBuffer::DecodeResult<>{
        GLOBED_UNWRAP_INTO(buf.readU32(), packetId);
        GLOBED_UNWRAP_INTO(buf.readU8(), frameIdx);
        GLOBED_UNWRAP_INTO(buf.readU8(), frameCount);

        return Ok();
    }();

    if (!rres) {
        return Err(fmt::to_string(rres.unwrapErr()));
    }

    auto& frames = packets[packetId];

    // do some sanity checks

    if (frameIdx >= frameCount) {
        return Err("frame index/count invalid");
    }

    for (const auto& frame : frames) {
        if (frame.idx == frameIdx) {
            return Err("repeated udp frame");
        }

        if (frame.max != frameCount) {
            return Err("mismatched frame idx/max");
        }
    }

    std::vector<uint8_t> data;
    size_t remainderLength = buf.size() - buf.getPosition();
    data.resize(remainderLength);

    auto result = buf.readBytesInto(data.data(), remainderLength);
    if (!result) {
        return Err(fmt::to_string(result.unwrapErr()));
    }

    frames.push_back(Frame {
        .idx = frameIdx,
        .max = frameCount,
        .data = std::move(data)
    });

    // check if we got all the needed frames
    if (frames.size() == frameCount) {
        std::sort(frames.begin(), frames.end(), [](auto& a, auto& b) {
            return a.idx < b.idx;
        });

        std::vector<uint8_t> data;
        
        // calculate full frame size
        size_t totalSize = 0;
        for (auto& frame : frames) {
            totalSize += frame.data.size();
        }

        data.reserve(totalSize);

        for (auto& frame : frames) {
            data.insert(data.end(), frame.data.begin(), frame.data.end());
        }

        return Ok(std::move(data));
    }

    return Ok(std::vector<uint8_t>{});
}

void UdpFrameBuffer::clear() {
    packets.clear();
}
