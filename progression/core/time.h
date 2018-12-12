#pragma once

#include "core/config.h"
#include <chrono>

namespace Progression {

class Time {
    friend class Window;
public:
    Time() = delete;
    ~Time() = delete;

    static void Init(const config::Config& config);
    static void Free() {}
    static float frameTime();
    static float totalTime();
    static float deltaTime();
	static unsigned int getFPS() { return _currentFPS; }
    static unsigned int totalFrameCount();
    static unsigned int currentFrameCount();
    static void showFPS(bool b);
    static void Restart();
    static std::chrono::high_resolution_clock::time_point getTimePoint();
    static double getDuration(const std::chrono::high_resolution_clock::time_point& point);

private:
    static void StartFrame();
    static void EndFrame();

	static unsigned _currentFPS;
    static float _mFrameTime;
    static float _mLastFrameTime;
    static float _mFPSTime;
    static float _mDeltaTime;
    static unsigned int _mCurrentFrameCount;
    static unsigned int _mTotalFrameCount;
    static bool _mDisplay;
};

} // namespace Progression