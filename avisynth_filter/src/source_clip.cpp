#include "pch.h"
#include "source_clip.h"
#include "logging.h"
#include "media_sample.h"


namespace AvsFilter {

SourceClip::SourceClip(const VideoInfo &videoInfo)
    : _videoInfo(videoInfo)
    , _flushOnNextInput(false)
    , _maxRequestedFrameNb(0) {
}

auto SourceClip::GetFrame(int frameNb, IScriptEnvironment *env) -> PVideoFrame {
    const std::unique_lock<std::mutex> lock(_bufferMutex);

    auto iter = _frameBuffer.cbegin();
    uint8_t frameCacheType = 1;
    if (frameNb >= iter->frameNb) {
        while (true) {
            if (iter == _frameBuffer.cend()) {
                --iter;
                frameCacheType = 4;
                break;
            } else if (frameNb < iter->frameNb) {
                --iter;
                frameCacheType = 3;
                break;
            } else if (frameNb == iter->frameNb) {
                frameCacheType = 2;
                break;
            }
            ++iter;
        }
    }

    _maxRequestedFrameNb = max(frameNb, _maxRequestedFrameNb);

    Log("GetFrame at: %6i Queue size: %2u Back: %6i Front: %6i Served(%u): %6i maxRequestFrame: %6i",
        frameNb, _frameBuffer.size(), _frameBuffer.cbegin()->frameNb, _frameBuffer.crbegin()->frameNb, frameCacheType, iter->frameNb, _maxRequestedFrameNb);

    return iter->frame;
}

auto SourceClip::GetParity(int frameNb) -> bool {
    return false;
}

auto SourceClip::GetAudio(void *buf, int64_t start, int64_t count, IScriptEnvironment *env) -> void {
}

auto SourceClip::SetCacheHints(int cachehints, int frame_range) -> int {
    return 0;
}

auto SourceClip::GetVideoInfo() -> const VideoInfo & {
    return _videoInfo;
}

auto SourceClip::PushBackFrame(PVideoFrame frame, REFERENCE_TIME startTime, const HDRSideData &hdrSideData) -> int {
    const std::unique_lock<std::mutex> lock(_bufferMutex);

    if (_flushOnNextInput) {
        _frameBuffer.clear();
        _maxRequestedFrameNb = 0;
        _flushOnNextInput = false;
    }

    if (!_frameBuffer.empty()) {
        _frameBuffer.rbegin()->stopTime = startTime;
    }

    const int frameNb = _frameBuffer.empty() ? 0 : _frameBuffer.crbegin()->frameNb + 1;
    _frameBuffer.emplace_back(FrameInfo { frameNb, frame, startTime, 0, hdrSideData });
    return frameNb;
}

auto SourceClip::GetFrontFrame() const -> std::optional<FrameInfo> {
    const std::unique_lock<std::mutex> lock(_bufferMutex);

    if (_frameBuffer.empty()) {
        return std::nullopt;
    }

    if (_frameBuffer.cbegin()->stopTime == 0) {
        return std::nullopt;
    }

    return *_frameBuffer.cbegin();
}

auto SourceClip::PopFrontFrame() -> void {
    const std::unique_lock<std::mutex> lock(_bufferMutex);

    const size_t dbgPreSize = _frameBuffer.size();

    if (!_frameBuffer.empty()) {
        _frameBuffer.erase(_frameBuffer.cbegin());
    }

    Log("Popping frame Pre size: %2u Post size: %2u", dbgPreSize, _frameBuffer.size());
}

auto SourceClip::FlushOnNextInput() -> void {
    const std::unique_lock<std::mutex> lock(_bufferMutex);

    _flushOnNextInput = true;
}

auto SourceClip::GetBufferSize() const -> int {
    const std::unique_lock<std::mutex> lock(_bufferMutex);

    return static_cast<int>(_frameBuffer.size());
}

auto SourceClip::GetMaxAccessedFrameNb() const -> int {
    const std::unique_lock<std::mutex> lock(_bufferMutex);

    return _maxRequestedFrameNb;
}

}