#include <iostream>
#include <iomanip>
#include "include/fps_counter.h"

FPSCounter::FPSCounter() {
    time_ = 0;
    frameCounter_ = 0;
    paused_ = 0;
    fpsTime_ = 0;
}

FPSCounter::~FPSCounter() {}

void FPSCounter::Start() {
    time_ = 0;
    frameCounter_ = 0;
    paused_ = false;
    fpsTime_ = 0;
}

void FPSCounter::StartFrame(float time) {
    if (!paused_) {
        time_ = time;
    }
}

void FPSCounter::EndFrame() {
    if (!paused_) {
        prevTime_ = time_;
        ++frameCounter_;
        if (time_ > fpsTime_ + 1) {
            std::cout << "FPS: " << frameCounter_ << std::endl;
            frameCounter_ = 0;
            fpsTime_ = time_;
        }
    }
}
