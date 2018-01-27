#ifndef SRC_INCLUDE_TIMER_H_
#define SRC_INCLUDE_TIMER_H_

class Timer {
    public:
        Timer();
        ~Timer();
        virtual void Start();
        virtual void Pause();
        virtual void Stop();
        virtual void Update(float dt);

    protected:
        float time_;
        bool paused_;
};

#endif  // SRC_INCLUDE_TIMER_H_
