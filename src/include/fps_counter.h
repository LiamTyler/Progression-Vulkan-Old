#pragma once

class FPSCounter {
    public:
        FPSCounter();
        ~FPSCounter();
        void Init();
        void StartFrame(float dt);
        void EndFrame();

        float GetDT() { return time_ - prevTime_; }
        void Display(bool b) { display_ = b; }

    protected:
        bool display_;
        float time_;
        float prevTime_;
        float fpsTime_;
        unsigned int frameCounter_;
};
