#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

class Profiler {
public:
    Profiler();

    void BeginFrame();
    void EndFrame();
    void BeginDrawSubmit();
    void EndDrawSubmit();
    void BeginWorldCalc();
    void EndWorldCalc();
    void BeginInstanceUpload();
    void EndInstanceUpload();
    void BeginDrawCall();
    void EndDrawCall();
    void RecordDrawCall();

    unsigned int DrawCalls() const { return drawCalls_; }
    double AverageFrameTimeMs() const;
    double DrawSubmitMs() const { return drawSubmitMs_; }
    double WorldCalcMs() const { return worldCalcMs_; }
    double InstanceUploadMs() const { return instanceUploadMs_; }
    double DrawCallMs() const { return drawCallMs_; }
    double LastFrameTimeMs() const { return lastFrameTimeMs_; }
    std::uint64_t FrameCount() const { return frameCount_; }

private:
    double ElapsedSince(std::int64_t start) const;

    static constexpr std::size_t kHistorySize = 60;

    std::int64_t frequency_ = 0;
    std::int64_t frameStart_ = 0;
    std::array<double, kHistorySize> frameTimesMs_ = {};
    std::size_t frameTimeIndex_ = 0;
    std::size_t frameTimeCount_ = 0;
    unsigned int drawCalls_ = 0;
    std::int64_t drawSubmitStart_ = 0;
    double drawSubmitMs_ = 0.0;
    std::int64_t worldCalcStart_ = 0;
    double worldCalcMs_ = 0.0;
    std::int64_t instanceUploadStart_ = 0;
    double instanceUploadMs_ = 0.0;
    std::int64_t drawCallStart_ = 0;
    double drawCallMs_ = 0.0;
    double lastFrameTimeMs_ = 0.0;
    std::uint64_t frameCount_ = 0;
};
