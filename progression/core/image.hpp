#pragma once

#include "core/common.hpp"

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
		Image(int w, int h);
		~Image();

        Image(const Image& src);
        Image& operator=(const Image& image);
        Image(Image&& src);
        Image& operator=(Image&& src);

		bool Load(const std::string& filename, bool flip_vertically = true);
		bool Save(const std::string& filename);

		int Width() const { return width_; }
		int Height() const { return height_; }
	    Pixel GetPixel(int r, int c) const {
            Pixel p;
            memcpy(&p, pixels_ + r * width_ + c, sizeof(Pixel));
            return p;
        }
		void SetPixel(int r, int c, const Pixel& p) { memcpy(pixels_ + r*width_ + c, &p, sizeof(p)); }
		unsigned char* GetData() const { return pixels_; }

	protected:
		int width_;
		int height_;
		unsigned char* pixels_;
	};

} // namespace Progression
