#pragma once

#include "core/common.hpp"
#include "resource/image.hpp"
#include "utils/noncopyable.hpp"
#include "resource/resource.hpp"

namespace Progression {

    class TextureMetaData : public MetaData {
    public:
        bool operator==(const TextureMetaData& meta) const;
        bool operator!=(const TextureMetaData& meta) const;
        friend std::ostream& operator<<(std::ostream& out, const TextureMetaData& meta) {
            return out << meta.file.filename << " " << meta.internalFormat << " " << meta.minFilter << " "
                    << meta.magFilter << " " << meta.wrapModeS << " " << meta.wrapModeT << " " <<
                    meta.mipMapped << " " << meta.freeCpuCopy;
        }

        TimeStampedFile file = TimeStampedFile();
        Image* image         = nullptr;
        GLint internalFormat = GL_SRGB;
        GLint minFilter      = GL_LINEAR;
        GLint magFilter      = GL_LINEAR;
        GLint wrapModeS      = GL_REPEAT;
        GLint wrapModeT      = GL_REPEAT;
        bool mipMapped       = true;
        bool freeCpuCopy     = true;
    };

    class Texture2D : public NonCopyable, public Resource {
    public:
        Texture2D();
        Texture2D(const std::string& name, const TextureMetaData& data);
        ~Texture2D();

        Texture2D(Texture2D&& texture);
        Texture2D& operator=(Texture2D&& texture);

        bool load(MetaData* metaData = nullptr) override;
        bool readMetaData(std::istream& in) override;
        ResUpdateStatus loadFromResourceFile(std::istream& in, std::function<void()>& updateFunc) override;
        void move(Resource* resource) override;
        std::shared_ptr<Resource> needsReloading() override;
        bool saveToFastFile(std::ofstream& outFile) const override;
        bool loadFromFastFile(std::ifstream& in) override;
        void uploadToGPU();
        
        GLuint gpuHandle() const { return gpuHandle_; }
        unsigned int width() const { return width_; }
        unsigned int height() const { return height_; }

        TextureMetaData metaData;

    private:
        GLuint gpuHandle_ = -1;
        int width_        = 0;
        int height_       = 0;
    };

} // namespace Progression
