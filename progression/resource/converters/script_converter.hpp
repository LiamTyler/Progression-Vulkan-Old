#include "resource/converters/converter.hpp"
#include "resource/script.hpp"
#include <string>

class ScriptConverter : public Converter
{
public:
    ScriptConverter() = default;

    AssetStatus CheckDependencies() override;
    ConverterStatus Convert() override;
    std::string GetName() const override;

    Progression::ScriptCreateInfo createInfo;
};
