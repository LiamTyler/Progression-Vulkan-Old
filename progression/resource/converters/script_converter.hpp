#include "resource/converters/converter.hpp"
#include "resource/script.hpp"
#include <string>

class ScriptConverter : public Converter
{
public:
    ScriptConverter( bool force_ = false, bool verbose_ = false );

    AssetStatus CheckDependencies() override;
    ConverterStatus Convert() override;
    std::string GetName() const override;

    Progression::ScriptCreateInfo createInfo;
};
