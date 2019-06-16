#pragma once

#include "resource/texture2D.hpp"
#include <fstream>

namespace Progression {

    bool loadTexture2D(Texture2D& tex, const std::string& fname, const TextureUsageDesc& desc, bool freeCPUCopy = true);

    bool getTextureInfoFromResourceFile(std::string& fname, TextureUsageDesc& desc, bool& freeCPUCopy, std::istream& in);

} // namespace Progression
