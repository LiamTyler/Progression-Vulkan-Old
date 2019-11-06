#pragma once

namespace Progression
{

class Scene;

namespace AnimationSystem
{

bool Init();

void Shutdown();

void Update( Scene* scene );

} // namespace AnimationSystem
} // namespace Progression