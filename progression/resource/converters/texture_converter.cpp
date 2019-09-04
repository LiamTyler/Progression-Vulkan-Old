#include "resource/converters/texture_converter.hpp"
#include "core/assert.hpp"
#include "resource/image.hpp"
#include "resource/resource_manager.hpp"
#include "resource/texture.hpp"
#include "utils/fileIO.hpp"
#include "utils/logger.hpp"
#include "utils/serialize.hpp"
#include "utils/timestamp.hpp"
#include <filesystem>
#include <fstream>

using namespace Progression;

static std::string GetTextureFastFileName( TextureCreateInfo& createInfo )
{
    namespace fs = std::filesystem;

    PG_ASSERT( !createInfo.filename.empty() );
    fs::path filePath = fs::absolute( createInfo.filename );

    size_t hash          = std::hash< std::string >{}( filePath.string() );
    std::string baseName = filePath.filename().string();

    return PG_RESOURCE_DIR "cache/textures/" + createInfo.name + "_" + baseName +
           std::to_string( hash ) + ".ffi";
}

AssetStatus TextureConverter::CheckDependencies()
{
    PG_ASSERT( !createInfo.filename.empty() );

    // Add an empty texture to the manager to notify other resources that an image with this
    // name has already been loaded, so it doesnt get loaded multiple times
    auto placeHolderTex = std::make_shared< Texture >();
    placeHolderTex->name = createInfo.name;
    ResourceManager::Add< Texture >( placeHolderTex );

    if ( outputFile.empty() )
    {
        outputFile = GetTextureFastFileName( createInfo );
    }
    if ( !std::filesystem::exists( outputFile ) )
    {
        LOG( "OUT OF DATE: FFI File for image with name '", createInfo.name, "' and filename '",
             createInfo.filename, "' does not exist, convert required" );
        return ASSET_OUT_OF_DATE;
    }
    if ( Timestamp( outputFile ) < Timestamp( createInfo.filename ) )
    {
        LOG( "OUT OF DATE: Image with name '", createInfo.name, "' and filename '",
             createInfo.filename, "' has newer timestamp than saved FFI" );
        return ASSET_OUT_OF_DATE;
    }

    std::ifstream in( outputFile, std::ios::binary );
    PG_ASSERT( in );

    TextureCreateInfo savedCreateInfo;
    serialize::Read( in, savedCreateInfo.name );
    serialize::Read( in, savedCreateInfo.texDesc.format );
    serialize::Read( in, savedCreateInfo.texDesc.type );
    serialize::Read( in, savedCreateInfo.texDesc.mipmapped );
    serialize::Read( in, savedCreateInfo.samplerDesc.minFilter );
    serialize::Read( in, savedCreateInfo.samplerDesc.magFilter );
    serialize::Read( in, savedCreateInfo.samplerDesc.wrapModeS );
    serialize::Read( in, savedCreateInfo.samplerDesc.wrapModeT );
    serialize::Read( in, savedCreateInfo.samplerDesc.wrapModeR );
    serialize::Read( in, savedCreateInfo.samplerDesc.borderColor );
    serialize::Read( in, savedCreateInfo.samplerDesc.maxAnisotropy );

    PG_ASSERT( createInfo.texDesc.type == savedCreateInfo.texDesc.type );

    PG_ASSERT( !in.fail() );
    in.close();

    if ( createInfo.texDesc.format != savedCreateInfo.texDesc.format ||
         createInfo.texDesc.mipmapped != savedCreateInfo.texDesc.mipmapped ||
         createInfo.samplerDesc.minFilter != savedCreateInfo.samplerDesc.minFilter ||
         createInfo.samplerDesc.magFilter != savedCreateInfo.samplerDesc.magFilter ||
         createInfo.samplerDesc.wrapModeS != savedCreateInfo.samplerDesc.wrapModeS ||
         createInfo.samplerDesc.wrapModeT != savedCreateInfo.samplerDesc.wrapModeT ||
         createInfo.samplerDesc.wrapModeR != savedCreateInfo.samplerDesc.wrapModeR ||
         createInfo.samplerDesc.maxAnisotropy != savedCreateInfo.samplerDesc.maxAnisotropy )
    {
        LOG( "OUT TO DATE: Image with name '", createInfo.name, "' and filename '",
             createInfo.filename, "' has same image, but newer metadata, resaving metadata." );
        m_justMetaDataOutOfDate = true;
        m_image                 = new Image;
        m_image->Deserialize( in );
        return ASSET_OUT_OF_DATE;
    }

    LOG( "UP TO DATE: Image with name '", createInfo.name, "' and filename '", createInfo.filename,
         "'" );

    m_status = ASSET_UP_TO_DATE;
    return m_status;
}

ConverterStatus TextureConverter::Convert()
{
    auto resPtr = std::make_shared< Texture >();
    resPtr->name = createInfo.name;
    ResourceManager::Add< Texture >( resPtr );
    if ( m_status == ASSET_UP_TO_DATE )
    {
        return CONVERT_SUCCESS;
    }

    std::ofstream out( outputFile, std::ios::binary );
    if ( !out )
    {
        LOG_ERR( "Could not open the FFI file '", outputFile, "' during texture convert" );
        if ( m_image )
        {
            delete m_image;
        }
        return CONVERT_ERROR;
    }

    serialize::Write( out, createInfo.name );

    serialize::Write( out, createInfo.texDesc.format );
    serialize::Write( out, createInfo.texDesc.type );
    serialize::Write( out, createInfo.texDesc.mipmapped );

    serialize::Write( out, createInfo.samplerDesc.minFilter );
    serialize::Write( out, createInfo.samplerDesc.magFilter );
    serialize::Write( out, createInfo.samplerDesc.wrapModeS );
    serialize::Write( out, createInfo.samplerDesc.wrapModeT );
    serialize::Write( out, createInfo.samplerDesc.wrapModeR );
    serialize::Write( out, createInfo.samplerDesc.borderColor );
    serialize::Write( out, createInfo.samplerDesc.maxAnisotropy );

    if ( m_justMetaDataOutOfDate )
    {
        m_image->Serialize( out );
        delete m_image;
    }
    else
    {
        Image img;
        if ( !img.Load( createInfo.filename ) )
        {
            out.close();
            return CONVERT_ERROR;
        }
        img.Serialize( out );
    }

    out.close();

    return CONVERT_SUCCESS;
}
