#include "core/time.hpp"
#include "core/common.hpp"

static float s_currentFrameStartTime = 0;
static float s_lastFrameStartTime    = 0;
static float s_deltaTime             = 0;

namespace Progression
{

namespace Time
{
    void Reset()
    {
        glfwSetTime( 0 );
        s_currentFrameStartTime = 0;
        s_deltaTime             = 0;
        s_currentFrameStartTime = 0;
    }

    float TotalTime()
    {
        return static_cast< float >( glfwGetTime() );
    }

    float DeltaTime()
    {
        return s_deltaTime;
    }

    void StartFrame()
    {
        s_currentFrameStartTime = TotalTime();
        s_deltaTime             = s_currentFrameStartTime - s_lastFrameStartTime;
    }

    void EndFrame()
    {
        s_lastFrameStartTime = s_currentFrameStartTime;
    }

    std::chrono::high_resolution_clock::time_point GetTimePoint()
    {
        return std::chrono::high_resolution_clock::now();
    }

    double GetDuration( const std::chrono::high_resolution_clock::time_point& point )
    {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast< std::chrono::microseconds >( now - point ).count() /
               (float) 1000;
    }

} // namespace Time
} // namespace Progression
