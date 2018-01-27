#ifndef SRC_INCLUDE_FPS_COUNTER_H_
#define SRC_INCLUDE_FPS_COUNTER_H_

#include "include/timer.h"

class FPSCounter : public Timer {
    public:
        FPSCounter();
        ~FPSCounter();
        void Start();
        void StartFrame(float dt);
        void EndFrame();
        float GetDT() { return time_ - prevTime_; }

    protected:
        float prevTime_;
        float fpsTime_;
        unsigned int frameCounter_;
};

#endif  // SRC_INCLUDE_FPS_COUNTER_H_

