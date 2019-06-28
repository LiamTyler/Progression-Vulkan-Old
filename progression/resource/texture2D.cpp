#include "resource/texture2D.hpp"
#include "utils/logger.hpp"

namespace Progression {

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
        gpuHandle_     = std::move(texture.gpuHandle_);
        width_         = std::move(texture.width_);
        height_        = std::move(texture.height_);
        metaData       = std::move(texture.metaData);

        texture.metaData.image  = nullptr;
        texture.gpuHandle_ = (GLuint) -1;

        return *this;
    }

    Resource* Texture2D::needsReloading() {
        Texture2D* tex = nullptr;
        if (metaData.file.update()) {
            tex = new Texture2D(name, metaData);
        }
        return tex;
    }

    bool Texture2D::load() {
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
            if (metaData.mipMapped)
                glGenerateMipmap(GL_TEXTURE_2D);
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

        if (metaData.freeCPUCopy) {
            delete image;
            metaData.image = nullptr;
        }
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

    bool Texture2D::loadMetaDataFromFile(std::istream& in) {
        std::string line;
        std::string s;
        std::istringstream ss;
        std::unordered_map<std::string, GLint>::iterator it;

        // texture name
        std::getline(in, line);
        ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == "name");
        ss >> name;
        PG_ASSERT(!in.fail() && !ss.fail());

        // texture name
        std::getline(in, line);
        ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == "filename");
        ss >> s;
        PG_ASSERT(!in.fail() && !ss.fail());
        metaData.file = TimeStampedFile(PG_RESOURCE_DIR + s);

        // freeCPUCopy
        std::getline(in, line);
        ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == "freeCPUCopy");
        ss >> s;
        PG_ASSERT(s == "true" || s == "false");
        metaData.freeCPUCopy = s == "true";
        PG_ASSERT(!in.fail() && !ss.fail());

        // mipmaped
        std::getline(in, line);
        ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == "mipmapped");
        ss >> s;
        PG_ASSERT(s == "true" || s == "false");
        metaData.mipMapped = s == "true";
        PG_ASSERT(!in.fail() && !ss.fail());

        // internal format
        std::getline(in, line);
        ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == "internalFormat");
        ss >> s;
        it = internalFormatMap.find(s);
        if (it == internalFormatMap.end()) {
            LOG_ERR("Invalid texture format: ", s);
            return false;
        }
        metaData.internalFormat = it->second;
        PG_ASSERT(!in.fail() && !ss.fail());


        // min filter
        std::getline(in, line);
        ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == "minFilter");
        ss >> s;
        it = minFilterMap.find(s);
        if (it == minFilterMap.end()) {
            LOG_ERR("Invalid minFilter format: ", s);
            return false;
        }
        if (!metaData.mipMapped && (s != "nearest" && s != "linear")) {
            LOG_WARN("Trying to use a mip map min filter when there is no mip map on texture: ", metaData.file.filename);
        }
        metaData.minFilter = it->second;
        PG_ASSERT(!in.fail() && !ss.fail());

        // mag filter
        std::getline(in, line);
        ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == "magFilter");
        ss >> s;
        it = magFilterMap.find(s);
        if (it == magFilterMap.end()) {
            LOG_ERR("Invalid magFilter format: ", s);
            return false;
        }
        metaData.magFilter = it->second;
        PG_ASSERT(!in.fail() && !ss.fail());

        // wrapModeS
        std::getline(in, line);
        ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == "wrapModeS");
        ss >> s;
        it = wrapMap.find(s);
        if (it == wrapMap.end()) {
            LOG_ERR("Invalid wrapModeS format: ", s);
            return false;
        }
        metaData.wrapModeS = it->second;
        PG_ASSERT(!in.fail() && !ss.fail());

        // wrapModeT
        std::getline(in, line);
        ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == "wrapModeT");
        ss >> s;
        it = wrapMap.find(s);
        if (it == wrapMap.end()) {
            LOG_ERR("Invalid wrapModeT format: ", s);
            return false;
        }
        metaData.wrapModeT = it->second;
        PG_ASSERT(!in.fail() && !ss.fail());

        return !in.fail() && !ss.fail();
    }

} // namespace Progression
