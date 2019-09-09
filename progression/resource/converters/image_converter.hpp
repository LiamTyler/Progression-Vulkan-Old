#include "resource/converters/converter.hpp"
#include "resource/image.hpp"
#include <string>

class ImageConverter : public Converter
{
public:
    ImageConverter() = default;

    AssetStatus CheckDependencies() override;
    ConverterStatus Convert() override;

    Progression::ImageCreateInfo createInfo;
};
