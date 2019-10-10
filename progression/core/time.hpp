#pragma once

#include <chrono>

namespace Progression
{

namespace Time
{

    void Reset();
    float Time();
    float DeltaTime();
    void StartFrame();
    void EndFrame();

    std::chrono::high_resolution_clock::time_point GetTimePoint();
    // Returns the number of milliseconds elapsed since the given point in time
    double GetDuration( const std::chrono::high_resolution_clock::time_point& point );

} // namespace Time
} // namespace Progression
