#pragma once

#include "resource/image.hpp"
#include <string>

namespace Progression {

    bool loadImage(Image& image, const std::string& fname, bool flipVertically = true);

    bool saveImage(const Image& image, const std::string& fname, bool flipVertically = true);

} // namespace Progression
