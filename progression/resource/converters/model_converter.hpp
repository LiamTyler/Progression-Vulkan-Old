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
};
