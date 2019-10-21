#include "resource/converters/converter.hpp"
#include "resource/converters/image_converter.hpp"
#include "resource/converters/material_converter.hpp"
#include "resource/converters/model_converter.hpp"
#include "resource/converters/script_converter.hpp"
#include "resource/converters/shader_converter.hpp"

class FastfileConverter : public Converter
{
public:
    FastfileConverter() = default;

    AssetStatus CheckDependencies();
    ConverterStatus Convert();

    std::string inputFile;

private:
    std::vector< ImageConverter >    m_imageConverters;
    std::vector< MaterialConverter > m_materialFileConverters;
    std::vector< ModelConverter >    m_modelConverters;
    std::vector< ScriptConverter >   m_scriptConverters;
    std::vector< ShaderConverter >   m_shaderConverters;
};
