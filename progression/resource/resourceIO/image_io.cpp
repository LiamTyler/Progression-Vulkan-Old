#include "resource/resourceIO/image_io.hpp"
#include "utils/logger.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image_write.h"

namespace Progression {

	bool loadImage(Image& image, const std::string& fname, bool flip_vertically) {
        stbi_set_flip_vertically_on_load(flip_vertically);
        int width, height, numComponents;
        unsigned char* pixels = stbi_load(fname.c_str(), &width, &height, &numComponents, 0);

        if (!pixels) {
            LOG_ERR("Failed to load image:", fname);
            return false;
        }
        image = std::move(Image(width, height, numComponents, pixels));

        return true;
	}

	bool saveImage(const Image& image, const std::string& fname) {
		int i = fname.length();
		while (fname[--i] != '.' && i >= 0);
		if (i < 0) {
            LOG_ERR("Image filename \"", fname, "\" has no extension");
			return false;
		}
        int width = image.width();
        int height = image.height();
        int numComponents = image.numComponents();
        unsigned char* pixels = image.pixels();

        int ret;
		switch (fname[i + 1]) {
		case 'p':
			ret = stbi_write_png(fname.c_str(), width, height, numComponents, pixels, width * numComponents);
			break;
		case 'j':
            ret = stbi_write_jpg(fname.c_str(), width, height, numComponents, pixels, 95);
			break;
		case 'b':
            ret = stbi_write_bmp(fname.c_str(), width, height, numComponents, pixels);
			break;
		case 't':
            ret = stbi_write_tga(fname.c_str(), width, height, numComponents, pixels);
			break;
		default:
            LOG_ERR("Cant save an image with an unrecognized format:", fname)
            return false;
		}
        if (!ret)
            LOG_ERR("failed to write image:", fname);

        return ret;
	}

} // namespace Progression
