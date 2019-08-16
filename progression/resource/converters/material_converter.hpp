#include "resource/converters/converter.hpp"
#include <string>

class MaterialConverter : public Converter
{
public:
    MaterialConverter() = default;

    AssetStatus CheckDependencies() override;
    ConverterStatus Convert() override;

    std::string inputFile;
};
