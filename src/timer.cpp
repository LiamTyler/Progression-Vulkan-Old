#include "include/timer.h"

Timer::Timer() {
    time_ = 0;
    paused_ = false;
}

Timer::~Timer() {}

void Timer::Start() {
    time_ = 0;
    paused_ = false;
}

void Timer::Pause() {
    paused_ = true;
}

void Timer::Stop() {
}

void Timer::Update(float dt) {
    if (!paused_) {
        time_ += dt;
    }
}
