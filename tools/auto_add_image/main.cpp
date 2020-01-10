#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "getopt/getopt.h"
#include "progression.hpp"
#include "utils/json_parsing.hpp"
#include "utils/string.hpp"
#include <filesystem>

using namespace Progression;
using namespace Gfx;
namespace fs = std::filesystem;

struct ImageEntry
{
    std::string semantic;
    std::string relPath; // relative to PG_RESOURCE_DIR
};

std::unordered_map< std::string, ImageEntry > images;
std::string diffuseFormat = "BC7_SRGB";
std::string normalFormat  = "BC5_UNORM";

static void DisplayHelp()
{
    auto msg =
      "Usage: auto_add_image RESOURCE_FILE.json\n"
      " or\n"
      "Usage: auto_add_image [--diffuseFormat|--normalFormat] modelFile1 modelFile2...\n"
      "\nOptions\n"
      "  -d, --diffuseFormat [format]\tSet the format of diffuse textures (defaults to BC7_SRGB). Use 'INVALID' for src format\n"
      "  -h, --help\t\tPrint this message and exit\n"
      "  -m, --mipsOff\tDon't generate mipmaps\n";
      "  -n, --normalFormat [format]\tSet the format of normal textures (defaults to BC5_UNORM). Use 'INVALID' for src format\n";
      "  -o, --output [filename]\tOutput filename when arguments are model files. Defaults to 'auto_add_image_output.json'\n";
      "  -q, --quality [0-3]\tSet the quality level. Defaults to 1, the higher the better.\n";
      "  -y, --flipY\tTurn OFF flipping textures vertically. Defaults to on'\n";

    std::cout << msg << std::endl;
}

static void AddImage( const aiMaterial* pMaterial, aiTextureType texType )
{
    std::string name;
    aiString path;
    if ( pMaterial->GetTexture( texType, 0, &path, NULL, NULL, NULL, NULL, NULL ) == AI_SUCCESS )
    {
        name = TrimWhiteSpace( path.data );
    }
    else
    {
        std::cout << "Could not parse texture path from aiMaterial" << std::endl;
    }
    PG_ASSERT( name != "" );
    if ( images.find( name ) != images.end() )
    {
        return;
    }

    std::string fullPath;
    if ( fs::exists( PG_RESOURCE_DIR + name ) )
    {
        fullPath = PG_RESOURCE_DIR + name;
    }
    else
    {
        std::string basename = fs::path( name ).filename().string();
        for( auto itEntry = fs::recursive_directory_iterator( PG_RESOURCE_DIR ); itEntry != fs::recursive_directory_iterator(); ++itEntry )
        {
            std::string itFile = itEntry->path().filename().string();
            if ( basename == itEntry->path().filename().string() )
            {
                fullPath = fs::absolute( itEntry->path() ).string();
                break;
            }
        }
    }
                    
    if ( fullPath != "" )
    {
        auto& entry    = images[name];
        entry.semantic = texType == aiTextureType_DIFFUSE ? "DIFFUSE" : "NORMAL";
        fs::path resDirPath( PG_RESOURCE_DIR );
        fs::path aiTexPath( fullPath );
        entry.relPath = fs::relative( aiTexPath, resDirPath ).string();
        std::replace( entry.relPath.begin(), entry.relPath.end(), '\\', '/');
    }
    else
    {
        std::cout << "Could not find image file '" << name << "'" << std::endl;
    }
}

static void OutputKeyStr( std::ostream& out, const std::string& key, const std::string& val, bool comma = true )
{
    out << "\t\t\"" << key << "\": " << "\"" << val << "\"";
    if ( comma )
    {
        out << ",";
    }
    out << "\n";
}

static void OutputKeyBool( std::ostream& out, const std::string& key, bool val, bool comma = true )
{
    std::string s = val ? "true" : "false";
    out << "\t\t\"" << key << "\": " << s;
    if ( comma )
    {
        out << ",";
    }
    out << "\n";
}

int main( int argc, char* argv[] )
{
    if ( argc == 1 )
    {
        DisplayHelp();
        return 0;
    }

    static struct option long_options[] =
    {
        { "diffuseFormat", required_argument,  0, 'd' },
        { "help",          no_argument,        0, 'h' },
        { "mipsOff",       no_argument,        0, 'm' },
        { "normalFormat",  required_argument,  0, 'n' },
        { "output",        required_argument,  0, 'o' },
        { "quality",       required_argument,  0, 'q' },
        { "flipY",         no_argument,        0, 'y' },
        { 0, 0, 0, 0 }
    };

    std::string outputFilename = "auto_add_image_output.json";
    bool flipY        = true;
    bool generateMips = true;
    bool helpMessage  = false;
    int qualityLevel  = 1;
    int option_index  = 0;
    int c             = -1;
    while ( ( c = getopt_long( argc, argv, "d:hmn:o:q:y", long_options, &option_index ) ) != -1 )
    {
        switch ( c )
        {
            case 'd':
            {
                std::string format = optarg;
                diffuseFormat      = format;
                auto gfxFormat     = PixelFormatFromString( format );
                if ( gfxFormat != PixelFormat::INVALID && !PixelFormatIsCompressed( gfxFormat ) )
                {
                    std::cout << "Cannot currently convert to non-compressed formats" << std::endl;
                    return 0;
                }
                if ( PixelFormatFromString( format ) == PixelFormat::INVALID && format != "INVALID" )
                {
                    std::cout << "Invalid pixel format '" << format << "'" << std::endl;
                    return 0;
                }
                break;
            }
            case 'h':
                helpMessage = true;
                break;
            case 'm':
                generateMips = false;
                break;
            case 'n':
            {
                std::string format = optarg;
                normalFormat       = format;
                auto gfxFormat     = PixelFormatFromString( format );
                if ( gfxFormat != PixelFormat::INVALID && !PixelFormatIsCompressed( gfxFormat ) )
                {
                    std::cout << "Cannot currently convert to non-compressed formats" << std::endl;
                    return 0;
                }
                if ( gfxFormat == PixelFormat::INVALID && format != "INVALID" )
                {
                    std::cout << "Invalid pixel format '" << format << "'" << std::endl;
                    return 0;
                }
                break;
            }
            case 'o':
                outputFilename = optarg;
                break;
            case 'q':
                qualityLevel = std::max( 0, std::min( 3, std::stoi( optarg ) ) );
                break;
            case 'y':
                flipY = false;
                break;
            case '?':
                std::cout << "Try 'auto_add_image --help for more information" << std::endl;
                return 0;
            default:
                break;
        }
    }

    if ( helpMessage || optind >= argc )
    {
        DisplayHelp();
        return 0;
    }

    if ( argc - optind == 1 && fs::path( argv[1] ).extension().string() == ".json" )
    {
        std::cout << "Have not yet implemented for resource files!" << std::endl;
    }
    else
    {
        for ( int i = optind; i < argc; ++i )
        {
            std::string modelFilename = argv[i];
            Assimp::Importer importer;
            const aiScene* scene = importer.ReadFile( modelFilename.c_str(), 0 );
            if ( !scene )
            {
                LOG_ERR( "Error parsing model file '", modelFilename, "': ", importer.GetErrorString() );
                return 0;
            }

            namespace fs = std::filesystem;
            for ( uint32_t mtlIdx = 0; mtlIdx < scene->mNumMaterials; ++mtlIdx )
            {
                const aiMaterial* pMaterial = scene->mMaterials[mtlIdx];
                if ( pMaterial->GetTextureCount( aiTextureType_DIFFUSE ) > 0 )
                {
                    PG_ASSERT( pMaterial->GetTextureCount( aiTextureType_DIFFUSE ) == 1, "Can't have more than 1 diffuse texture per material" );
                    AddImage( pMaterial, aiTextureType_DIFFUSE );
                }

                if ( pMaterial->GetTextureCount( aiTextureType_NORMALS ) > 0 )
                {
                    PG_ASSERT( pMaterial->GetTextureCount( aiTextureType_NORMALS ) == 1, "Can't have more than 1 normal map per material" );
                    AddImage( pMaterial, aiTextureType_NORMALS );
                }
                else if ( pMaterial->GetTextureCount( aiTextureType_HEIGHT ) > 0 )
                {
                    PG_ASSERT( pMaterial->GetTextureCount( aiTextureType_HEIGHT ) == 1, "Can't have more than 1 normal map per material" );
                    AddImage( pMaterial, aiTextureType_HEIGHT );
                }
            }

        }

        std::ofstream out( outputFilename );
        if ( !out )
        {
            std::cout << "Could not open output file '" << outputFilename << "'" << std::endl;
            return 0;
        }

        std::string qualityStrs[] =
        {
            "LOW",
            "MEDIUM",
            "HIGH",
            "MAX"
        };
    
        for ( const auto& [ name, entry ] : images )
        {
            out << "\t\"Image\": {\n";
            OutputKeyStr( out, "name", name );
            OutputKeyStr( out, "filename", entry.relPath );
            OutputKeyStr( out, "semantic", entry.semantic );
            auto dstFormat = entry.semantic == "DIFFUSE" ? diffuseFormat : normalFormat;
            if ( dstFormat != "INVALID" )
            {
                OutputKeyStr( out, "dstFormat", dstFormat );
                OutputKeyStr( out, "compressionQuality", qualityStrs[qualityLevel] );
            }
            OutputKeyBool( out, "generateMipmaps", generateMips );
            OutputKeyBool( out, "flipVertically", flipY );
            OutputKeyBool( out, "freeCpuCopy",   true );
            OutputKeyBool( out, "createTexture", true, false );
            out << "\t}," << std::endl;
        }
        out.close();
    }

    return 0;
}
