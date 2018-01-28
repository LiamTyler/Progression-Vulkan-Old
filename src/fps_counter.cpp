#include <iostream>
#include <iomanip>
#include "include/fps_counter.h"

FPSCounter::FPSCounter() {
    time_ = 0;
    frameCounter_ = 0;
    fpsTime_ = 0;
}

FPSCounter::~FPSCounter() {}

void FPSCounter::Init() {
    time_ = 0;
    frameCounter_ = 0;
    fpsTime_ = 0;
}

void FPSCounter::StartFrame(float time) {
    time_ = time;
}

void FPSCounter::EndFrame() {
    prevTime_ = time_;
    ++frameCounter_;
    if (time_ > fpsTime_ + 1) {
        std::cout << "FPS: " << frameCounter_ << std::endl;
        frameCounter_ = 0;
        fpsTime_ = time_;
    }
}
