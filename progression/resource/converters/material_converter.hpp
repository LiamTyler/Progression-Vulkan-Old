#include "resource/converters/converter.hpp"
#include <string>

class MaterialConverter : public Converter
{
public:
    MaterialConverter() = default;

    AssetStatus CheckDependencies() override;
    ConverterStatus Convert() override;
    virtual bool WriteToFastFile( std::ofstream& out ) const override;
    std::string GetName() const override;

    std::string inputFile;
    uint32_t numMaterials;
};
