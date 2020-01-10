#include "resource/converters/converter.hpp"
#include "resource/shader.hpp"
#include <string>

class ShaderConverter : public Converter
{
public:
    ShaderConverter( bool force_ = false, bool verbose_ = false );

    AssetStatus CheckDependencies() override;
    ConverterStatus Convert() override;
    std::string GetName() const override;

    Progression::ShaderCreateInfo createInfo;
};
