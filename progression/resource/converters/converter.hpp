#pragma once

#include "getopt.h"
#include <string>
#include <vector>

enum ConverterStatus
{
    CONVERT_SUCCESS,
    CONVERT_HELP_DISPLAYED,
    CONVERT_PARSE_ERROR,
    CONVERT_ERROR,
};

enum AssetStatus
{
    ASSET_UP_TO_DATE,
    ASSET_OUT_OF_DATE,
    ASSET_CHECKING_ERROR,
};

#define IF_VERBOSE_MODE( x ) \
    if ( verbose ) \
    { \
        do { x; } while( false ); \
    }

class Converter
{
public:
    Converter()          = default;
    virtual ~Converter() = default;

    // returns true if up-to-date, false if out-of-date
    virtual AssetStatus CheckDependencies() = 0;

    // returns 0 on success
    virtual ConverterStatus Convert() = 0;

    virtual bool WriteToFastFile( std::ofstream& out, bool debugMode ) const;

    virtual std::string GetName() const;

    bool force         = false;
    bool verbose       = false;
    AssetStatus status = ASSET_OUT_OF_DATE;

protected:
    std::string m_outputContentFile;
    std::string m_outputSettingsFile;
    bool  m_contentNeedsConverting;
    bool  m_settingsNeedsConverting;
};
