#pragma once

#include "core/common.h"

namespace Progression {

	class Image {
	public:
		Image();
		Image(int w, int h);
		Image(const std::string filename);
		Image(const Image& src);
		~Image();

		bool LoadImage(const std::string& filename);
		void SaveImage(const std::string& filename);

		int Width() const { return width_; }
		int Height() const { return height_; }
		glm::vec4 GetPixel(int r, int c) const { return pixels_[r*width_ + c]; }
		void SetPixel(int r, int c, glm::vec4 p) { pixels_[r*width_ + c] = p; }
		glm::vec4* GetData() const { return pixels_; }

		static glm::vec4 ClampPixel(glm::vec4 p);
		static float ClampFloat(float f);
		static float Luminance(glm::vec4 p);

	protected:
		int width_;
		int height_;
		glm::vec4* pixels_;
	};

} // namespace Progression