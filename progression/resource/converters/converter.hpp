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

class Converter
{
public:
    Converter()          = default;
    virtual ~Converter() = default;

    // returns true if up-to-date, false if out-of-date
    virtual AssetStatus CheckDependencies() = 0;

    // returns 0 on success
    virtual ConverterStatus Convert() = 0;

    virtual bool WriteToFastFile( std::ofstream& out ) const;

    virtual std::string GetName() const;
    AssetStatus GetStatus() const;

protected:
    std::string m_outputContentFile;
    std::string m_outputSettingsFile;
    bool  m_contentNeedsConverting;
    bool  m_settingsNeedsConverting;
    AssetStatus m_status = ASSET_OUT_OF_DATE;
};
