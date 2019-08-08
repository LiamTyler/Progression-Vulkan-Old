#include "resource/converters/converter.hpp"
#include "resource/model.hpp"
#include <string>

class ModelConverter : public Converter
{
public:
    ModelConverter() = default;

    AssetStatus CheckDependencies();
    ConverterStatus Convert();

    struct Progression::ModelCreateInfo createInfo;
    std::string outputFile;

private:
    AssetStatus status = ASSET_OUT_OF_DATE;
};
