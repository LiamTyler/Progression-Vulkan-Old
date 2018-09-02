#include "core/time.h"
#include "core/common.h"

namespace Progression {

    float Time::_mFrameTime = 0;
    float Time::_mLastFrameTime = 0;
    float Time::_mDeltaTime = 0;
    float Time::_mFPSTime = 0;
    unsigned int Time::_mCurrentFrameCount = 0;
    unsigned int Time::_mTotalFrameCount = 0;
    bool Time::_mDisplay = false;

    void Time::Init(const config::Config& config) {
        glfwSetTime(0);
        _mFrameTime = 0;
        _mDeltaTime = 0;
        _mFPSTime = 0;
        _mCurrentFrameCount = 0;
        _mTotalFrameCount = 0;
        _mDisplay = true;
        auto& timeConf = config["time"];
        if (timeConf) {
            if (timeConf["displayFPS"])
                _mDisplay = timeConf["displayFPS"].as<bool>();
        }
    }

    float Time::frameTime() { return _mFrameTime; }

    float Time::totalTime() { return glfwGetTime(); }

    float Time::deltaTime() { return _mDeltaTime; }

    unsigned int Time::totalFrameCount() { return _mTotalFrameCount; }

    unsigned int Time::currentFrameCount() { return _mCurrentFrameCount; }

    void Time::showFPS(bool b) { _mDisplay = b; }

    void Time::StartFrame() {
        _mFrameTime = totalTime();
        _mDeltaTime = _mFrameTime - _mLastFrameTime;
    }

    void Time::EndFrame() {
        _mLastFrameTime = _mFrameTime;
        ++_mCurrentFrameCount;
        ++_mTotalFrameCount;
        if (_mFrameTime > _mFPSTime + 1) {
            if (_mDisplay)
                std::cout << "FPS: " << _mCurrentFrameCount << std::endl;
            _mCurrentFrameCount = 0;
            _mFPSTime = _mFrameTime;
        }
    }

} // namespace Progression