#include "resource/image.hpp"
#include <cstdlib>
#include <utility>
#include "utils/logger.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image_write.h"

namespace Progression {

    Pixel::Pixel() : r(0), g(0), b(0), a(0) {}
    Pixel::Pixel(unsigned char _r, unsigned char _g, unsigned char _b, unsigned char _a) : 
        r(_r), g(_g), b(_b), a(_a)
    {
    }

	Image::Image() :
		width_(0),
		height_(0),
		pixels_(nullptr)
    {
	}

	Image::Image(int w, int h, int nc, unsigned char* pixels) :
		width_(w),
		height_(h),
        numComponents_(nc),
        pixels_(pixels)
    {
        if (!pixels_)
            pixels_ = (unsigned char*) malloc(nc*w*h);
	}

    Image::~Image() {
        if (pixels_)
            free(pixels_);
    }

	Image::Image(const Image& src) {
        *this = src;
	}

    Image& Image::operator=(const Image& image) {
        if (pixels_)
            free(pixels_);

        if (!image.pixels_) {
            pixels_ = nullptr;
            width_ = 0;
            height_ = 0;
            numComponents_ = 0;
        } else {
            width_ = image.width_;
            height_ = image.height_;
            numComponents_ = image.numComponents_;
            pixels_ = (unsigned char*) malloc(numComponents_ * width_ * height_);
            memcpy(pixels_, image.pixels_, numComponents_ * width_ * height_);
        }

        return *this;
    }

    Image::Image(Image&& src) {
        *this = std::move(src);
    }

    Image& Image::operator=(Image&& src) {
        if (pixels_)
            free(pixels_);
        width_ = src.width_;
        height_ = src.height_;
        numComponents_ = src.numComponents_;
        pixels_ = src.pixels_;
        src.pixels_ = nullptr;
        return *this;
    }

    bool Image::load(const std::string& fname, bool flip_vertically) {
        stbi_set_flip_vertically_on_load(flip_vertically);
        int width, height, numComponents;
        unsigned char* pixels = stbi_load(fname.c_str(), &width, &height, &numComponents, 0);
        LOG("num components = ", numComponents);

        if (!pixels) {
            LOG_ERR("Failed to load image:", fname);
            return false;
        }
        *this = std::move(Image(width, height, numComponents, pixels));

        return true;
	}

	bool Image::save(const std::string& fname, bool flipVertically) {
        stbi_flip_vertically_on_write(flipVertically);
		int i = fname.length();
		while (fname[--i] != '.' && i >= 0);
		if (i < 0) {
            LOG_ERR("Image filename \"", fname, "\" has no extension");
			return false;
		}

        int ret;
		switch (fname[i + 1]) {
		case 'p':
			ret = stbi_write_png(fname.c_str(), width_, height_, numComponents_, pixels_, width_ * numComponents_);
			break;
		case 'j':
            ret = stbi_write_jpg(fname.c_str(), width_, height_, numComponents_, pixels_, 95);
			break;
		case 'b':
            ret = stbi_write_bmp(fname.c_str(), width_, height_, numComponents_, pixels_);
			break;
		case 't':
            ret = stbi_write_tga(fname.c_str(), width_, height_, numComponents_, pixels_);
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
