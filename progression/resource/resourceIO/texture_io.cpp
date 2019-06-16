#include "resource/resourceIO/texture_io.hpp"
#include "resource/resourceIO/image_io.hpp"
#include "utils/logger.hpp"
#include "core/common.hpp"

namespace Progression {

    bool loadTexture2D(Texture2D& texture, const std::string& fname, const TextureUsageDesc& desc, bool freeCPUCopy) {
        Image* img = new Image;
        if (!loadImage(*img, fname)) {
            LOG_ERR("Could not load texture image: ", fname);
            return false;
        }
        texture = std::move(Texture2D(img, desc, freeCPUCopy));

        return true;
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

    // typedef struct TextureUsageDesc {
    //     GLint internalFormat = GL_SRGB;
    //     GLint minFilter      = GL_LINEAR;
    //     GLint magFilter      = GL_LINEAR;
    //     GLint wrapModeS      = GL_REPEAT;
    //     GLint wrapModeT      = GL_REPEAT;
    //     bool mipMapped       = true;
    // } TextureUsageDesc;
    bool getTextureInfoFromResourceFile(std::string& fname, TextureUsageDesc& desc, bool& freeCPUCopy, std::istream& in) {
        std::string line;
        std::string s;
        std::istringstream ss;
        std::unordered_map<std::string, GLint>::iterator it;

        // texture name
        std::getline(in, line);
        ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == "filename");
        ss >> fname;
        PG_ASSERT(!in.fail() && !ss.fail());

        // freeCPUCopy
        std::getline(in, line);
        ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == "freeCPUCopy");
        ss >> s;
        PG_ASSERT(s == "true" || s == "false");
        freeCPUCopy = s == "true";
        PG_ASSERT(!in.fail() && !ss.fail());

        // mipmaped
        std::getline(in, line);
        ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == "mipmapped");
        ss >> s;
        PG_ASSERT(s == "true" || s == "false");
        desc.mipMapped = s == "true";
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
        desc.internalFormat = it->second;
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
        if (!desc.mipMapped && (s != "nearest" && s != "linear")) {
            LOG_WARN("Trying to use a mip map min filter when there is no mip map on texture: ", fname);
        }
        desc.minFilter = it->second;
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
        desc.magFilter = it->second;
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
        desc.wrapModeS = it->second;
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
        desc.wrapModeT = it->second;
        PG_ASSERT(!in.fail() && !ss.fail());

        return true;
    }

} // namespace Progression
