#include "resource/texture2D.hpp"
#include "utils/logger.hpp"
#include "resource/resource_manager.hpp"
#include "utils/serialize.hpp"
#include "utils/fileIO.hpp"

namespace Progression {

    bool TextureMetaData::operator==(const TextureMetaData& meta) const {
        return file.filename == meta.file.filename &&
               internalFormat == meta.internalFormat &&
               minFilter == meta.minFilter &&
               magFilter == meta.magFilter &&
               wrapModeS == meta.wrapModeS &&
               wrapModeT == meta.wrapModeT &&
               mipMapped == meta.mipMapped &&
               freeCpuCopy == meta.freeCpuCopy;
    }

    bool TextureMetaData::operator!=(const TextureMetaData& meta) const {
        return !(*this == meta);
    }

    Texture2D::Texture2D() :
        Resource("")
    {
        glGenTextures(1, &gpuHandle_);
    }

    Texture2D::Texture2D(const std::string& _name, const TextureMetaData& data) :
        Resource(_name),
        metaData(data)
    {
        glGenTextures(1, &gpuHandle_);
    }

    Texture2D::~Texture2D() {
        if (metaData.image)
            delete metaData.image;
        if (gpuHandle_ != (GLuint) -1)
            glDeleteTextures(1, &gpuHandle_);
    }

    Texture2D::Texture2D(Texture2D&& texture) {
        *this = std::move(texture);
    }

    Texture2D& Texture2D::operator=(Texture2D&& texture) {
        name           = std::move(texture.name);
        gpuHandle_     = std::move(texture.gpuHandle_);
        width_         = std::move(texture.width_);
        height_        = std::move(texture.height_);
        metaData       = std::move(texture.metaData);

        texture.metaData.image  = nullptr;
        texture.gpuHandle_ = (GLuint) -1;

        return *this;
    }

    std::shared_ptr<Resource> Texture2D::needsReloading() {
        if (metaData.file.update()) {
            return std::make_shared<Texture2D>(name, metaData);
        }
        return nullptr;
    }

    bool Texture2D::load(MetaData* data) {
        if (data)
            metaData = *(TextureMetaData*) data;
        
        Image* img = new Image;
        if (!img->load(metaData.file.filename)) {
            LOG_ERR("Could not load texture image: ", metaData.file.filename);
            delete img;
            return false;
        }

        if (metaData.image)
            delete metaData.image;
        metaData.image = img;
        uploadToGPU();

        return true;
    }

    // TODO: Find out if mipmaps need to be regenerated
    void Texture2D::uploadToGPU() {
        Image* image = metaData.image;
        if (!image)
            return;

        glBindTexture(GL_TEXTURE_2D, gpuHandle_);
        static const GLenum formats[] = { GL_RED, GL_RG, GL_RGB, GL_RGBA };

        // check to see if the image has a new size. If so, create new gpu image
        if (width_ != image->width() || height_ != image->height()) {
            width_ = image->width();
            height_ = image->height();

            glTexImage2D(
                    GL_TEXTURE_2D,
                    0,
                    metaData.internalFormat,
                    width_,
                    height_,
                    0,
                    formats[image->numComponents() - 1],
                    GL_UNSIGNED_BYTE,
                    image->pixels()
                    );
        } else { // otherwise just stream the data
            glTexSubImage2D(
                    GL_TEXTURE_2D,
                    0,
                    0,
                    0,
                    width_,
                    height_,
                    GL_UNSIGNED_BYTE,
                    formats[image->numComponents() - 1],
                    image->pixels()
                    );
        }
        if (metaData.mipMapped)
            glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, metaData.wrapModeS);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, metaData.wrapModeT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, metaData.minFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, metaData.magFilter);

        if (metaData.freeCpuCopy) {
            delete image;
            metaData.image = nullptr;
        }
    }

    void Texture2D::move(Resource* resource) {
        Texture2D& newShader = *(Texture2D*) resource;
        newShader = std::move(*this);
    }

    static std::unordered_map<std::string, GLint> internalFormatMap = {
        { "R8",      GL_R8 },
        { "RG8",     GL_RG},
        { "RGB8",    GL_RGB },
        { "RGBA8",   GL_RGBA },
        { "R16F",    GL_R16F },
        { "RG16F",   GL_RG16F },
        { "RGB16F",  GL_RGB16F },
        { "RGBA16F", GL_RGBA16F },
        { "R32F",    GL_R32F },
        { "RG32F",   GL_RG32F },
        { "RGB32F",  GL_RGB32F },
        { "RGBA32F", GL_RGBA32F },
        { "SRGB8",   GL_SRGB8 },
        { "SRGBA8",  GL_SRGB8_ALPHA8 },
        { "DEPTH",   GL_DEPTH_COMPONENT },
    };

    static std::unordered_map<std::string, GLint> minFilterMap = {
        { "nearest", GL_NEAREST },
        { "linear", GL_LINEAR },
        { "nearest_mipmap_nearest", GL_NEAREST_MIPMAP_NEAREST },
        { "linear_mipmap_nearest", GL_LINEAR_MIPMAP_NEAREST },
        { "nearest_mipmap_linear", GL_NEAREST_MIPMAP_LINEAR },
        { "linear_mipmap_linear", GL_LINEAR_MIPMAP_LINEAR },
    };

    static std::unordered_map<std::string, GLint> magFilterMap = {
        { "nearest", GL_NEAREST },
        { "linear", GL_LINEAR },
    };

    static std::unordered_map<std::string, GLint> wrapMap = {
        { "clamp_to_edge", GL_CLAMP_TO_EDGE },
        { "clamp_to_border", GL_CLAMP_TO_BORDER },
        { "mirror_repeat", GL_MIRRORED_REPEAT },
        { "repeat", GL_REPEAT },
        { "mirror_clamp_to_edge", GL_MIRROR_CLAMP_TO_EDGE },
    };

    bool Texture2D::readMetaData(std::istream& in) {
        std::string line;
        std::string tmp;
 
        decltype(auto) parseMapValue = [&](const auto& map, const char* key) {
            std::string tmpStr;
            fileIO::parseLineKeyVal(in, key, tmpStr);
            auto it = map.find(tmpStr);
            if (it == map.end()) {
                LOG_ERR("Invalid texture format: ", tmp);
                return false;
            }
            
            PG_ASSERT(!in.fail());
            return it->second;
        };

        fileIO::parseLineKeyVal(in, "name", name);
        fileIO::parseLineKeyVal(in, "filename", tmp);
        metaData.file = TimeStampedFile(PG_RESOURCE_DIR + tmp);
        metaData.freeCpuCopy = fileIO::parseLineKeyBool(in, "freeCpuCopy");
        metaData.mipMapped = fileIO::parseLineKeyBool(in, "mipmapped");
        if (!fileIO::parseLineKeyMap(in, "internalFormat", internalFormatMap, metaData.internalFormat)) {
            LOG("Invalid texture2D 'internalFormat'");
            return false;
        }
        if (!fileIO::parseLineKeyMap(in, "minFilter", minFilterMap, metaData.minFilter)) {
            LOG("Invalid texture2D 'minFilter'");
            return false;
        }
        if (!metaData.mipMapped && (metaData.minFilter != GL_NEAREST && metaData.minFilter != GL_LINEAR)) {
            LOG_WARN("Trying to use a mip map min filter when there is no mip map on texture: ", metaData.file.filename);
        }
        if (!fileIO::parseLineKeyMap(in, "magFilter", magFilterMap, metaData.magFilter)) {
            LOG("Invalid texture2D 'magFilter'");
            return false;
        }
        if (!fileIO::parseLineKeyMap(in, "wrapModeS", wrapMap, metaData.wrapModeS)) {
            LOG("Invalid texture2D 'wrapModeS'");
            return false;
        }
        if (!fileIO::parseLineKeyMap(in, "wrapModeT", wrapMap, metaData.wrapModeT)) {
            LOG("Invalid texture2D 'wrapModeT'");
            return false;
        }

        return !in.fail();
    }

    ResUpdateStatus Texture2D::loadFromResourceFile(std::istream& in, std::function<void()>& updateFunc) {
        if (!readMetaData(in))
            return RES_PARSE_ERROR;

        auto currTex = std::static_pointer_cast<Texture2D>(ResourceManager::get<Texture2D>(name));
        if (currTex) {
            if (currTex->metaData.file.outOfDate(metaData.file)) {
                return load() ? RES_RELOAD_SUCCESS : RES_RELOAD_FAILED;
            } else if (currTex->metaData != metaData) {
                updateFunc = [metaData = metaData, currTex]() {
                    glBindTexture(GL_TEXTURE_2D, currTex->gpuHandle());
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, metaData.wrapModeS);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, metaData.wrapModeT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, metaData.minFilter);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, metaData.magFilter);
                    if (!currTex->metaData.mipMapped && metaData.mipMapped) {
                        glGenerateMipmap(GL_TEXTURE_2D);
                    }
                };
                return RES_UPDATED;
            } else {
                return RES_UP_TO_DATE;
            }
        }

        return load() ? RES_RELOAD_SUCCESS : RES_RELOAD_FAILED;
    }

    bool Texture2D::saveToFastFile(std::ofstream& out) const {
        serialize::write(out, name);
        metaData.file.save(out);
        serialize::write(out, metaData.internalFormat);
        serialize::write(out, metaData.minFilter);
        serialize::write(out, metaData.magFilter);
        serialize::write(out, metaData.wrapModeS);
        serialize::write(out, metaData.wrapModeT);
        serialize::write(out, metaData.mipMapped);
        serialize::write(out, metaData.freeCpuCopy);

        if (!metaData.image) {
            Image image;
            if (!image.load(metaData.file.filename)) {
                LOG_ERR("Could not load the image with filename: ", metaData.file.filename, " for texture: ", name);
                return false;
            }
            return image.save(out);
        }
        
        return metaData.image->save(out);
    }
    bool Texture2D::loadFromFastFile(std::ifstream& in) {
        serialize::read(in, name);
        metaData.file.load(in);
        serialize::read(in, metaData.internalFormat);
        serialize::read(in, metaData.minFilter);
        serialize::read(in, metaData.magFilter);
        serialize::read(in, metaData.wrapModeS);
        serialize::read(in, metaData.wrapModeT);
        serialize::read(in, metaData.mipMapped);
        serialize::read(in, metaData.freeCpuCopy);

        metaData.image = new Image;
        if (!metaData.image->load(in)) {
            LOG_ERR("Could not load the image for texture: ", name);
            delete metaData.image;
            metaData.image = nullptr;
            return false;
        }

        uploadToGPU();

        return !in.fail();
    }

} // namespace Progression
