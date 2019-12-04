#include "resource/converters/converter.hpp"
#include "resource/model.hpp"
#include <string>

class ModelConverter : public Converter
{
public:
    ModelConverter( bool force_ = false, bool verbose_ = false );

    AssetStatus CheckDependencies() override;
    ConverterStatus Convert() override;
    std::string GetName() const override;

    struct Progression::ModelCreateInfo createInfo;
};
