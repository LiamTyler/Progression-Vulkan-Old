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
    void UpdateStatus( AssetStatus s );

    std::string inputFile;

    std::vector< ImageConverter >    imageConverters;
    std::vector< MaterialConverter > materialFileConverters;
    std::vector< ModelConverter >    modelConverters;
    std::vector< ScriptConverter >   scriptConverters;
    std::vector< ShaderConverter >   shaderConverters;
};
