#include "resource/converters/converter.hpp"
#include "resource/converters/material_converter.hpp"
//#include "include/model_converter.hpp"
#include "resource/converters/shader_converter.hpp"
#include "resource/converters/texture_converter.hpp"

class FastfileConverter : public Converter
{
public:
    FastfileConverter() = default;

    AssetStatus CheckDependencies();
    ConverterStatus Convert();

    std::string inputFile;

private:
    std::vector< MaterialConverter > m_materialFileConverters;
    std::vector< ShaderConverter > m_shaderConverters;
    std::vector< TextureConverter > m_textureConverters;

    // std::vector< ModelConverter > m_modelConverters;
};
