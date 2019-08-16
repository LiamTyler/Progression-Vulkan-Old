#include "resource/converters/converter.hpp"
#include "resource/texture.hpp"
#include <string>

namespace Progression
{

class Image;

} // namespace Progression

class TextureConverter : public Converter
{
public:
    TextureConverter() = default;

    AssetStatus CheckDependencies() override;
    ConverterStatus Convert() override;

    Progression::TextureCreateInfo createInfo;

private:
    Progression::Image* m_image  = nullptr;
    bool m_justMetaDataOutOfDate = false;
};
