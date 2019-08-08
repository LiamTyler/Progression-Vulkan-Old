#include "progression.hpp"
#include "resource/converters/fastfile_converter.hpp"
#include <filesystem>

using namespace Progression;

static void DisplayHelp()
{
    auto msg =
      "Usage: converter [--force|--verbose] <resource type> <specific options for resource "
      "conversion...>\n"
      "\nSpecify which type of resource is to be converted into its fastfile specific\n"
      "format, followed by the options specific to converting that resource, seen with:\n"
      "  converter <resource type> --help\n\n"
      "Options\n"
      "  --fastfile\t\tConvert a resource file\n"
      "  --force\t\tConvert the resource without checking dependencies\n"
      "  -h, --help\t\tPrint this message and exit\n"
      "  -i, --image\t\tConvert an image\n"
      "  -m, --model\t\tConvert a model\n"
      "  -s, --shader\t\tConvert a shader\n"
      "  -v, --verbose\t\tPrint out details during resource conversion\n";

    std::cout << msg << std::endl;
}

// static ConverterStatus ConvertShader( int argc, char** argv, bool force, bool verbose )
// {
//     ShaderConverter conv;
//     return conv.Run( argc, argv, force, verbose );
// }

// static ConverterStatus ConvertFastfile( int argc, char** argv, bool force, bool verbose )
// {
//     FastfileConverter conv;
//     return conv.Run( argc, argv, force, verbose );
// }

int main( int argc, char* argv[] )
{
    PG_UNUSED( argc );
    PG_UNUSED( argv );
    PG::EngineInitialize( PG_ROOT_DIR "configs/offline.toml" );

    namespace fs = std::filesystem;
    fs::create_directories( PG_RESOURCE_DIR "cache/shaders/" );
    fs::create_directories( PG_RESOURCE_DIR "cache/materials/" );
    fs::create_directories( PG_RESOURCE_DIR "cache/models/" );
    fs::create_directories( PG_RESOURCE_DIR "cache/textures/" );
    fs::create_directories( PG_RESOURCE_DIR "fastfiles/" );


    FastfileConverter conv;
    conv.inputFile = PG_RESOURCE_DIR "scenes/resource.txt";
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

    // bool force   = false;
    // bool verbose = false;

    // static struct option long_options[] = {
    //     { "fastfile", no_argument, 0, 'b' }, { "force", no_argument, 0, 'f' },
    //     { "help", no_argument, 0, 'h' },     { "image", no_argument, 0, 'i' },
    //     { "model", no_argument, 0, 'm' },    { "shader", no_argument, 0, 's' },
    //     { "verbose", no_argument, 0, 'v' },  { 0, 0, 0, 0 }
    // };

    // int option_index = 0;
    // int c            = -1;

    // const ConverterStatus noConversionYet = (ConverterStatus)-123456789;
    // ConverterStatus retVal                = noConversionYet;
    // while ( retVal == noConversionYet &&
    //         ( c = getopt_long( argc, argv, "himsv", long_options, &option_index ) ) != -1 )
    // {
    //     switch ( c )
    //     {
    //         case 'b':
    //             // retVal = ConvertFastfile( argc, argv, force, verbose );
    //             break;
    //         case 'f':
    //             force = true;
    //             break;
    //         case 'h':
    //             DisplayHelp();
    //             retVal = CONVERT_HELP_DISPLAYED;
    //             break;
    //         case 'i':
    //             LOG( "Need to parse an image" );
    //             break;
    //         case 'm':
    //             LOG( "Need to parse a model" );
    //             break;
    //         case 's':
    //             // retVal = ConvertShader( argc, argv, force, verbose );
    //             break;
    //         case 'v':
    //             verbose = true;
    //             break;
    //         default:
    //             break;
    //     }
    // }

    // if ( retVal != CONVERT_SUCCESS && retVal != CONVERT_HELP_DISPLAYED )
    // {
    //     LOG_ERR( "Failed to convert resource with error code: ", retVal );
    // }

    PG::EngineQuit();

    return 0;
}
