#pragma once

#include "resource/texture2D.hpp"

namespace Progression {

    bool loadTexture2D(Texture2D& tex, const std::string& fname, const TextureUsageDesc& desc, bool freeCPUCopy = true);

} // namespace Progression
