#pragma once
/*
class FPSCounter {
public:
    FPSCounter() : display_(true), time_(0), prevTime_(0), fpsTime_(0), frameCounter_(0) {}
    ~FPSCounter() = default;

    void StartFrame(float t) { time_ = t; }

    void EndFrame() {
        prevTime_ = time_;
        ++frameCounter_;
        if (time_ > fpsTime_ + 1) {
            if (display_)
                std::cout << "FPS: " << frameCounter_ << std::endl;
            frameCounter_ = 0;
            fpsTime_ = time_;
        }
    }

    float GetDT() const { return time_ - prevTime_; }
    void Display(bool b) { display_ = b; }

private:
    bool display_;
    float time_;
    float prevTime_;
    float fpsTime_;
    unsigned int frameCounter_;
};
*/

namespace Progression {

class Time {
    friend class Window;
public:
    Time() = delete;
    ~Time() = delete;

    static void Init(bool fps = true);
    static void Free() {}
    static float frameTime();
    static float totalTime();
    static float deltaTime();
    static unsigned int totalFrameCount();
    static unsigned int currentFrameCount();
    static void showFPS(bool b);

private:
    static void StartFrame();
    static void EndFrame();

    static float _mFrameTime;
    static float _mLastFrameTime;
    static float _mFPSTime;
    static float _mDeltaTime;
    static unsigned int _mCurrentFrameCount;
    static unsigned int _mTotalFrameCount;
    static bool _mDisplay;

};

} // namespace Progression