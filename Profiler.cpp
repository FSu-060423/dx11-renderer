#include "Profiler.h"

#include <windows.h>

Profiler::Profiler() {
    LARGE_INTEGER frequency = {};
    if (QueryPerformanceFrequency(&frequency) != 0) {
        frequency_ = frequency.QuadPart;
    }
}

void Profiler::BeginFrame() {
    drawCalls_ = 0;
    drawSubmitMs_ = 0.0;
    worldCalcMs_ = 0.0;
    instanceUploadMs_ = 0.0;
    drawCallMs_ = 0.0;

    LARGE_INTEGER now = {};
    QueryPerformanceCounter(&now);
    frameStart_ = now.QuadPart;
}

double Profiler::ElapsedSince(std::int64_t start) const {
    if (frequency_ <= 0) {
        return 0.0;
    }

    LARGE_INTEGER now = {};
    QueryPerformanceCounter(&now);
    return static_cast<double>(now.QuadPart - start) * 1000.0 /
           static_cast<double>(frequency_);
}

void Profiler::BeginDrawSubmit() {
    LARGE_INTEGER now = {};
    QueryPerformanceCounter(&now);
    drawSubmitStart_ = now.QuadPart;
}

void Profiler::EndDrawSubmit() {
    drawSubmitMs_ += ElapsedSince(drawSubmitStart_);
}

void Profiler::BeginWorldCalc() {
    LARGE_INTEGER now = {};
    QueryPerformanceCounter(&now);
    worldCalcStart_ = now.QuadPart;
}

void Profiler::EndWorldCalc() {
    worldCalcMs_ += ElapsedSince(worldCalcStart_);
}

void Profiler::BeginInstanceUpload() {
    LARGE_INTEGER now = {};
    QueryPerformanceCounter(&now);
    instanceUploadStart_ = now.QuadPart;
}

void Profiler::EndInstanceUpload() {
    instanceUploadMs_ += ElapsedSince(instanceUploadStart_);
}

void Profiler::BeginDrawCall() {
    LARGE_INTEGER now = {};
    QueryPerformanceCounter(&now);
    drawCallStart_ = now.QuadPart;
}

void Profiler::EndDrawCall() {
    const double elapsed = ElapsedSince(drawCallStart_);
    drawCallMs_ += elapsed;
    drawSubmitMs_ += elapsed;
}

void Profiler::EndFrame() {
    LARGE_INTEGER now = {};
    QueryPerformanceCounter(&now);

    if (frequency_ > 0) {
        lastFrameTimeMs_ =
            static_cast<double>(now.QuadPart - frameStart_) * 1000.0 /
            static_cast<double>(frequency_);
    } else {
        lastFrameTimeMs_ = 0.0;
    }

    frameTimesMs_[frameTimeIndex_] = lastFrameTimeMs_;
    frameTimeIndex_ = (frameTimeIndex_ + 1) % kHistorySize;
    if (frameTimeCount_ < kHistorySize) {
        ++frameTimeCount_;
    }

    ++frameCount_;
}

void Profiler::RecordDrawCall() {
    ++drawCalls_;
}

double Profiler::AverageFrameTimeMs() const {
    if (frameTimeCount_ == 0) {
        return 0.0;
    }

    double total = 0.0;
    for (std::size_t i = 0; i < frameTimeCount_; ++i) {
        total += frameTimesMs_[i];
    }
    return total / static_cast<double>(frameTimeCount_);
}
