#include "progression.hpp"
#include "resource/converters/fastfile_converter.hpp"
#include <filesystem>

using namespace Progression;

static void DisplayHelp()
{
    auto msg =
      "Usage: converter [--force|--verbose] RESOURCE_FILE\n"
      "\nOptions\n"
      "  -f, --force\t\tConvert the resource without checking dependencies\n"
      "  -h, --help\t\tPrint this message and exit\n"
      "  -v, --verbose\t\tPrint out details during resource conversion\n";

    std::cout << msg << std::endl;
}

int main( int argc, char* argv[] )
{
    if ( argc == 1 )
    {
        DisplayHelp();
        return 0;
    }

    g_converterMode = true;
    PG::EngineInitialize( PG_ROOT_DIR "configs/offline.toml" );

    namespace fs = std::filesystem;
    fs::create_directories( PG_RESOURCE_DIR "cache/shaders/" );
    fs::create_directories( PG_RESOURCE_DIR "cache/materials/" );
    fs::create_directories( PG_RESOURCE_DIR "cache/models/" );
    fs::create_directories( PG_RESOURCE_DIR "cache/images/" );
    fs::create_directories( PG_RESOURCE_DIR "cache/fastfiles/" );

    bool force   = false;
    bool verbose = false;

    static struct option long_options[] = {
        { "help", no_argument, 0, 'h' },
        { "force", no_argument, 0, 'f' },
        { "verbose", no_argument, 0, 'v' },
        { 0, 0, 0, 0 }
    };

    bool helpMessage = false;
    int option_index = 0;
    int c            = -1;
    while ( ( c = getopt_long( argc, argv, "hfv", long_options, &option_index ) ) != -1 )
    {
        switch ( c )
        {
            case 'f':
                force = true;
                break;
            case 'h':
                helpMessage = true;
                break;
            case 'v':
                verbose = true;
                break;
            case '?':
                std::cout << "Try 'converter --help for more information" << std::endl;
                PG::EngineQuit();
                return 0;
            default:
                break;
        }
    }

    if ( helpMessage || optind >= argc )
    {
        DisplayHelp();
    }
    else
    {
        FastfileConverter conv;
        conv.inputFile = PG_RESOURCE_DIR + std::string( argv[optind] );
        conv.force     = force;
        conv.verbose   = verbose;
        LOG( "Checking fastfile dependencies..." );
        AssetStatus status = conv.CheckDependencies();
        if ( status == ASSET_CHECKING_ERROR )
        {
            LOG_ERR( "Error while checking fastfile depends" );
        }
        else if ( status == ASSET_UP_TO_DATE )
        {
            LOG( "Fastfile up to date already" );
        }
        else
        {
            LOG( "Fastfile out of date, reconverting..." );
            if ( CONVERT_ERROR == conv.Convert() )
            {
                LOG_ERR( "Fastfile conversion failed" );
            }
        }
    }

    PG::EngineQuit();

    return 0;
}
