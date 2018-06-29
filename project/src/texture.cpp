#include "include/texture.h"

namespace Progression {

	Texture::Texture() :
		filename_(""),
		mip_mapped_(false),
		gpu_handle_(-1),
		width_(0),
		height_(0)
	{
	}

	Texture::Texture(const std::string& fname, bool mip_mapped) {
		Load(fname, mip_mapped);
	}

	Texture::Texture(const Image& image, bool mip_mapped) {
		Load(image, mip_mapped);
	}

	// TODO: Consider copy and assignment
	Texture::~Texture() {
		Free();
	}

	bool Texture::Load(const std::string& fname, bool mip_mapped) {
		filename_ = fname;
		mip_mapped_ = mip_mapped;
		Image image;
		if (!image.LoadImage(filename_))
			return false;
		LoadImpl(image);
		return true;
	}

	bool Texture::Load(const Image& image, bool mip_mapped) {
		filename_ = "";
		mip_mapped_ = mip_mapped;
		LoadImpl(image);
		return true;
	}

	void Texture::Free() {
		if (gpu_handle_ == -1) {
			glDeleteTextures(1, &gpu_handle_);
			filename_ = "";
			mip_mapped_ = true;
		}
	}

	void Texture::LoadImpl(const Image& image) {
		width_ = image.Width();
		height_ = image.Height();
		glGenTextures(1, &gpu_handle_);
		glBindTexture(GL_TEXTURE_2D, gpu_handle_);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.Width(), image.Height(), 0, GL_RGBA,
			GL_FLOAT, image.GetData());
		if (mip_mapped_) {
			glGenerateMipmap(GL_TEXTURE_2D);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
				GL_LINEAR_MIPMAP_LINEAR);
		}
		else {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

} // namespace Progression