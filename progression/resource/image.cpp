#include "resource/image.hpp"
#include <cstdlib>
#include <utility>

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
        width_ = src.width_;
        height_ = src.height_;
        numComponents_ = src.numComponents_;
        pixels_ = src.pixels_;
        src.pixels_ = nullptr;
        return *this;
    }

} // namespace Progression
