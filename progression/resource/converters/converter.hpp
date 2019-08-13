#pragma once

#include "getopt.h"
#include <string>
#include <vector>

enum ConverterStatus : int
{
    CONVERT_SUCCESS,
    CONVERT_HELP_DISPLAYED,
    CONVERT_PARSE_ERROR,
    CONVERT_ERROR,
};

enum AssetStatus : int
{
    ASSET_UP_TO_DATE,
    ASSET_OUT_OF_DATE,
    ASSET_CHECKING_ERROR,
};

class ConvertHeader
{
public:
    virtual void serialize( std::ofstream& out );
    virtual void deserialize( std::ifstream& in );

    std::string outputFile;
    std::vector< std::string > fileDependencies;
};

class Converter
{
public:
    Converter()          = default;
    virtual ~Converter() = default;

    // returns true if up-to-date, false if out-of-date
    virtual AssetStatus CheckDependencies() = 0;

    // returns 0 on success
    virtual ConverterStatus Convert() = 0;

    AssetStatus GetStatus() const;

    std::string outputFile;
    bool force   = false;
    bool verbose = true;

protected:
    AssetStatus m_status = ASSET_OUT_OF_DATE;
};
