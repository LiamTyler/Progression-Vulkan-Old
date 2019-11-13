#include "resource/converters/converter.hpp"
#include "resource/image.hpp"
#include <string>

class ImageConverter : public Converter
{
public:
    ImageConverter( bool force = false, bool verbose = false );

    AssetStatus CheckDependencies() override;
    ConverterStatus Convert() override;
    std::string GetName() const override;

    Progression::ImageCreateInfo createInfo;
};
