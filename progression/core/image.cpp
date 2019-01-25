#include "core/image.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image_write.h"

#include <iostream>
#include <iomanip>
#include "utils/logger.hpp"

namespace Progression {

    Pixel::Pixel() : r(0), g(0), b(0), a(0) {}
    Pixel::Pixel(unsigned char _r, unsigned char _g, unsigned char _b, unsigned char _a) : 
        r(_r), g(_g), b(_b), a(_a) {}

	Image::Image() :
		width_(0),
		height_(0),
		pixels_(nullptr)
    {
	}

	Image::Image(int w, int h) :
		width_(w),
		height_(h),
        pixels_((unsigned char*) malloc(4*w*h))
    {
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
        } else {
            width_ = image.width_;
            height_ = image.height_;
            pixels_ = (unsigned char*) malloc(4 * width_ * height_);
            memcpy(pixels_, image.pixels_, 4 * width_ * height_);
        }

        return *this;
    }

    Image::Image(Image&& src) {
        *this = std::move(src);
    }

    Image& Image::operator=(Image&& src) {
        width_ = src.width_;
        height_ = src.height_;
        pixels_ = src.pixels_;
        src.pixels_ = nullptr;
        return *this;
    }

	bool Image::Load(const std::string& fname, bool flip_vertically) {
		int nC;
        stbi_set_flip_vertically_on_load(flip_vertically);
        pixels_ = stbi_load(fname.c_str(), &width_, &height_, &nC, 4);

        if (!pixels_) {
            LOG_ERR("Failed to load image:", fname);
            return false;
        }
        return true;
	}

	bool Image::Save(const std::string& fname) {
		int i = fname.length();
		while (fname[--i] != '.' && i >= 0);
		if (i < 0) {
            LOG_ERR("Image filename \"", fname, "\" has no extension");
			return false;
		}

        int ret;
		switch (fname[i + 1]) {
		case 'p':
			ret = stbi_write_png(fname.c_str(), width_, height_, 4, pixels_, width_ * 4);
			break;
		case 'j':
            ret = stbi_write_jpg(fname.c_str(), width_, height_, 4, pixels_, 95);
			break;
		case 'b':
            ret = stbi_write_bmp(fname.c_str(), width_, height_, 4, pixels_);
			break;
		case 't':
            ret = stbi_write_tga(fname.c_str(), width_, height_, 4, pixels_);
			break;
		default:
            LOG_ERR("Cant save image with filename (unrecognized extension):", fname)
            return false;
		}
        if (!ret)
            LOG_ERR("failed to write image:", fname);

        return ret;
	}

} // namespace Progression
