#pragma once

namespace Progression
{

class NonCopyable
{
public:
    NonCopyable()          = default;
    virtual ~NonCopyable() = default;

    NonCopyable( const NonCopyable& ) = delete;
    NonCopyable& operator=( const NonCopyable& ) = delete;

    NonCopyable( NonCopyable&& ) = default;
    NonCopyable& operator=( NonCopyable&& ) = default;
};

} // namespace Progression