#pragma once

#include "include/utils.h"
#include "include/image.h"

namespace Progression {

	class Texture {
	public:
		Texture();
		Texture(const std::string& fname, bool mip_mapped = false);
		Texture(const Image& image, bool mip_mapped = false);
		~Texture();

		bool Load(const std::string& fname, bool mip_mapped = false);
		bool Load(const Image& image, bool mip_mapped = false);
		void Free();

		std::string GetName() const { return filename_; }
		GLuint GetHandle() const { return gpu_handle_; }
		int GetWidth() const { return width_; }
		int GetHeight() const { return height_; }
		bool IsMipMapped() const { return mip_mapped_; }
		bool IsLoaded() const { return gpu_handle_ != -1; }

	protected:
		void LoadImpl(const Image& image);

		std::string filename_;
		GLuint gpu_handle_;
		int width_;
		int height_;
		bool mip_mapped_;
	};

} // namespace Progression