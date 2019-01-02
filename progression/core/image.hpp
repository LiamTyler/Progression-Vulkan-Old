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
		Image(const std::string filename);
		~Image();

        Image(const Image& src);
        Image& operator=(const Image& image);
        Image(Image&& src);
        Image& operator=(Image&& src);

		bool LoadImage(const std::string& filename);
		void SaveImage(const std::string& filename);

		int Width() const { return width_; }
		int Height() const { return height_; }
	    Pixel GetPixel(int r, int c) const {
            Pixel p;
            memcpy(&p, pixels_ + r * width_ + c, sizeof(Pixel));
            return p;
        }
		void SetPixel(int r, int c, const Pixel& p) { memcpy(pixels_ + r*width_ + c, &p, sizeof(p)); }
		unsigned char* GetData() const { return pixels_; }
		
        //static glm::vec4 ClampPixel(Pixel p);
		//static float ClampFloat(float f);
		//static float Luminance(glm::vec4 p);

	protected:
		int width_;
		int height_;
		unsigned char* pixels_;
	};

} // namespace Progression