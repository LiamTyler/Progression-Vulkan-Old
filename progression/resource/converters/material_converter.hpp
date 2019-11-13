#include "resource/converters/converter.hpp"
#include <string>

class MaterialConverter : public Converter
{
public:
    MaterialConverter( bool force_ = false, bool verbose_ = false );

    AssetStatus CheckDependencies() override;
    ConverterStatus Convert() override;
    virtual bool WriteToFastFile( std::ofstream& out ) const override;
    std::string GetName() const override;

    std::string inputFile;
    uint32_t numMaterials;
};
