#include "resource/converters/converter.hpp"
#include "resource/shader.hpp"
#include <string>

class ShaderConverter : public Converter
{
public:
    ShaderConverter() = default;

    AssetStatus CheckDependencies() override;
    ConverterStatus Convert() override;

    struct Progression::ShaderCreateInfo createInfo;
};
