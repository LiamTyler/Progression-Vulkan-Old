#include "resource/resourceIO/texture_io.hpp"

namespace Progression {

    bool loadTexture2D(Texture2D& texture, const std::string& fname, const TextureUsageDesc& desc, bool freeCPUCopy = true) {
        Image* img = new Image;
        if (!loadImage(*img, fname)) {
            LOG_ERR("Could not load texture image: ", fname);
            return false;
        }
        texture = std::move(Texture2D(img, desc, freeCPUCopy));

        return true;
    }

} // namespace Progression
