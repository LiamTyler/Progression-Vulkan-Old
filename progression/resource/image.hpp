#pragma once
#include <cstring>
#include <string>

namespace Progression {

    class Pixel {
    public:
        Pixel();
        Pixel(unsigned char r, unsigned char g, unsigned char b, unsigned char a);

        unsigned char r;
        unsigned char g;
        unsigned char b;
        unsigned char a;
    };

	class Image {
	public:
		Image();
		Image(int w, int h, int nc, unsigned char* pixels = nullptr);
		~Image();

        Image(const Image& src);
        Image& operator=(const Image& image);
        Image(Image&& src);
        Image& operator=(Image&& src);

        bool load(const std::string& fname, bool flipVertically = true);
        bool save(const std::string& fname, bool flipVertically = true);

		int width() const { return width_; }
		int height() const { return height_; }
		int numComponents() const { return numComponents_; }
		unsigned char* pixels() const { return pixels_; }

	    Pixel getPixel(int r, int c) const {
            Pixel p;
            memcpy(&p, &pixels_[numComponents_ * (r * width_ + c)], numComponents_);
            return p;
        }

		void setPixel(int r, int c, const Pixel& p) {
            memcpy(pixels_ + numComponents_ * (r * width_ + c), &p, numComponents_);
        }

	protected:
		int width_;
		int height_;
        int numComponents_;
		unsigned char* pixels_;
	};

} // namespace Progression
