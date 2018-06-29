#define STB_IMAGE_IMPLEMENTATION
#include "include/stb_image.h"
#include "include/image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "include/stb_image_write.h"

#include <iostream>
#include <iomanip>

namespace Progression {

	Image::Image() {
		width_ = 0;
		height_ = 0;
		pixels_ = nullptr;
	}

	Image::Image(int w, int h) {
		width_ = w;
		height_ = h;
		pixels_ = new glm::vec4[w*h];
		for (int i = 0; i < w*h; i++)
			pixels_[i] = glm::vec4(0, 0, 0, 0);
	}

	Image::Image(const std::string fname) {
		LoadImage(fname);
	}

	Image::Image(const Image& src) {
		if (pixels_ == nullptr || width_ != src.width_ || height_ != src.height_) {
			width_ = src.width_;
			height_ = src.height_;
			if (pixels_)
				delete[] pixels_;

			pixels_ = new glm::vec4[width_*height_];
		}
		
		for (int i = 0; i < width_*height_; i++)
			pixels_[i] = src.pixels_[i];
	}

	Image::~Image() {
		if (pixels_)
			delete[] pixels_;
	}

	bool Image::LoadImage(const std::string& fname) {
		int nC;
		unsigned char *raw = stbi_load(fname.c_str(), &width_, &height_, &nC, 4);
		if (raw == NULL) {
			std::cout << "Failed to load : " << fname << std::endl;
			return false;
		}
		else {
			pixels_ = new glm::vec4[width_*height_];
			int I = 0;
			for (int r = 0; r < height_; r++) {
				for (int c = 0; c < width_; c++) {
					float rr = static_cast<float>(raw[I++]) / 255.0f;
					float gg = static_cast<float>(raw[I++]) / 255.0f;
					float bb = static_cast<float>(raw[I++]) / 255.0f;
					float aa = static_cast<float>(raw[I++]) / 255.0f;
					SetPixel(r, c, glm::vec4(rr, gg, bb, aa));
				}
			}
		}
		return true;
	}

	void Image::SaveImage(const std::string& fname) {
		unsigned char* out = new unsigned char[width_*height_ * 4];
		int b = 0;
		for (int r = 0; r < height_; r++) {
			for (int c = 0; c < width_; c++) {
				glm::vec4 p = GetPixel(r, c);
				p = ClampPixel(p);
				out[b++] = static_cast<unsigned char>(255 * p.r);
				out[b++] = static_cast<unsigned char>(255 * p.g);
				out[b++] = static_cast<unsigned char>(255 * p.b);
				out[b++] = static_cast<unsigned char>(255 * p.a);
			}
		}


		int i = fname.length();
		while (fname[--i] != '.' && i >= 0);
		if (i < 0) {
			std::cout << "Invalid image file name: " << fname << std::endl;
			delete[] out;
			return;
		}

		switch (fname[i + 1]) {
		case 'p':
			stbi_write_png(fname.c_str(), width_, height_, 4, out, width_ * 4);
			break;
		case 'j':
			stbi_write_jpg(fname.c_str(), width_, height_, 4, out, 95);
			break;
		case 'b':
			stbi_write_bmp(fname.c_str(), width_, height_, 4, out);
			break;
		case 't':
			stbi_write_tga(fname.c_str(), width_, height_, 4, out);
			break;
		default:
			std::cout << "Invalid image file name: " << fname << std::endl;
		}
		delete[] out;
	}

	glm::vec4 Image::ClampPixel(glm::vec4 p) {
		return glm::vec4(
			ClampFloat(p.r),
			ClampFloat(p.g),
			ClampFloat(p.b),
			ClampFloat(p.a));
	}

	float Image::ClampFloat(float f) {
		return std::fmax(0.0f, std::fmin(1.0f, f));
	}

	float Image::Luminance(glm::vec4 p) {
		return .2126f * p.r + .7152f * p.g + .0722f * p.b;
	}

} // namespace Progression
