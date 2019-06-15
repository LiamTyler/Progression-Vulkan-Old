#include "resource/resourceIO/texture_io.hpp"
#include "resource/resourceIO/image_io.hpp"
#include "utils/logger.hpp"

namespace Progression {

    bool loadTexture2D(Texture2D& texture, const std::string& fname, const TextureUsageDesc& desc, bool freeCPUCopy) {
        Image* img = new Image;
        if (!loadImage(*img, fname)) {
            LOG_ERR("Could not load texture image: ", fname);
            return false;
        }
        texture = Texture2D(img, desc, freeCPUCopy);

        return true;
    }

} // namespace Progression
