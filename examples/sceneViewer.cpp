#include "progression.hpp"
#include <thread>
#include "basis_universal/transcoder/basisu_transcoder.h"
#include "memory_map/MemoryMapped.h"

using namespace Progression;

bool g_paused = false;

basist::transcoder_texture_format PGToBasisBCPixelFormat( Gfx::PixelFormat pgFormat )
{
    PG_ASSERT( Gfx::PixelFormatIsCompressed( pgFormat ) );
    basist::transcoder_texture_format convert[] =
    {
        basist::transcoder_texture_format::cTFBC1_RGB, // BC1_RGB_UNORM
        basist::transcoder_texture_format::cTFBC1_RGB, // BC1_RGB_SRGB
        basist::transcoder_texture_format::cTFTotalTextureFormats, // BC1_RGBA_UNORM
        basist::transcoder_texture_format::cTFTotalTextureFormats, // BC1_RGBA_SRGB
        basist::transcoder_texture_format::cTFTotalTextureFormats, // BC2_UNORM
        basist::transcoder_texture_format::cTFTotalTextureFormats, // BC2_SRGB
        basist::transcoder_texture_format::cTFBC3_RGBA, // BC3_UNORM
        basist::transcoder_texture_format::cTFBC3_RGBA, // BC3_SRGB
        basist::transcoder_texture_format::cTFBC4_R, // BC4_UNORM
        basist::transcoder_texture_format::cTFBC4_R, // BC4_SNORM
        basist::transcoder_texture_format::cTFBC5_RG, // BC5_UNORM (note: X = R and Y = Alpha)
        basist::transcoder_texture_format::cTFBC5_RG, // BC5_SNORM (note: X = R and Y = Alpha)
        basist::transcoder_texture_format::cTFTotalTextureFormats, // BC6H_UFLOAT
        basist::transcoder_texture_format::cTFTotalTextureFormats, // BC6H_SFLOAT
        basist::transcoder_texture_format::cTFBC7_M6_RGB, // BC7_UNORM
        basist::transcoder_texture_format::cTFBC7_M6_RGB, // BC7_SRGB
    };

    return convert[static_cast< int >( pgFormat ) - static_cast< int >( Gfx::PixelFormat::BC1_RGB_UNORM )]; 
}

bool TranscodeBasisFile( const std::string& filename, Gfx::PixelFormat dstFormat, Gfx::Texture& tex )
{
    MemoryMapped memMappedFile;
    if ( !memMappedFile.open( filename, MemoryMapped::WholeFile, MemoryMapped::SequentialScan ) )
    {
        LOG_ERR( "Could not open basis file: '", filename, "'" );
        return false;
    }

    auto start = Time::GetTimePoint();

    void* data               = (void*) memMappedFile.getData();
    uint32_t dataSizeInBytes = (uint32_t) memMappedFile.size();

    basist::etc1_global_selector_codebook sel_codebook( basist::g_global_selector_cb_size, basist::g_global_selector_cb );
    basist::basisu_transcoder transcoder( &sel_codebook );
    if ( !transcoder.start_transcoding( data, dataSizeInBytes ) )
    {
        LOG_ERR( "Could not start transcoding" );
        return false;
    }

    basist::transcoder_texture_format dstBasisTexFormat = PGToBasisBCPixelFormat( dstFormat );
    uint32_t dstTexFormatSize                           = Gfx::SizeOfPixelFromat( dstFormat );

    basist::basis_texture_type basisTextureType = transcoder.get_texture_type( data, dataSizeInBytes );
    LOG( "Texture type: ", basisTextureType );
    uint32_t imageCount = transcoder.get_total_images( data, dataSizeInBytes );
    basist::basisu_image_info firstImageInfo;
    transcoder.get_image_info( data, dataSizeInBytes, firstImageInfo, 0 );

    Gfx::ImageDescriptor imageDesc;
    if ( basisTextureType == basist::cBASISTexType2D )
    {
        imageDesc.type = Gfx::ImageType::TYPE_2D;
    }
    else if ( basisTextureType == basist::cBASISTexType2DArray )
    {
        imageDesc.type = Gfx::ImageType::TYPE_2D_ARRAY;
    }
    else if ( basisTextureType == basist::cBASISTexTypeCubemapArray )
    {
        if ( imageCount == 6 )
        {
            imageDesc.type = Gfx::ImageType::TYPE_CUBEMAP;
        }
        else
        {
            imageDesc.type = Gfx::ImageType::TYPE_CUBEMAP_ARRAY;
        }
    }
    else
    {
        PG_ASSERT( false, "Invalid texture type" );
    }
    imageDesc.arrayLayers = imageCount;
    imageDesc.width       = firstImageInfo.m_width;
    imageDesc.height      = firstImageInfo.m_height;
    imageDesc.mipLevels   = firstImageInfo.m_total_levels;
    imageDesc.format      = dstFormat;
    imageDesc.usage       = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    size_t totalImageSize = 0;
    for ( uint32_t imageIndex = 0; imageIndex < imageCount; ++imageIndex )
    {
        basist::basisu_image_info imageInfo;
        transcoder.get_image_info( data, dataSizeInBytes, imageInfo, imageIndex );
        PG_ASSERT( imageInfo.m_width == firstImageInfo.m_width );
        PG_ASSERT( imageInfo.m_height == firstImageInfo.m_height );
        PG_ASSERT( imageInfo.m_total_levels == firstImageInfo.m_total_levels );

        uint32_t width  = imageInfo.m_width;
        uint32_t height = imageInfo.m_height;

        for ( uint32_t mipLevel = 0; mipLevel < imageInfo.m_total_levels; ++mipLevel )
        {
            basist::basisu_image_level_info levelInfo;
            transcoder.get_image_level_info( data, dataSizeInBytes, levelInfo, imageIndex, mipLevel );
            PG_ASSERT( width == levelInfo.m_width && height == levelInfo.m_height );
            width  = ( width  / 2 + 3 ) & ~3;
            height = ( height / 2 + 3 ) & ~3;
            totalImageSize += levelInfo.m_total_blocks * dstTexFormatSize;
        }
    }
    char* allTranscodedData = static_cast< char* >( malloc( totalImageSize ) );
    char* currentSlice      = allTranscodedData;
    for ( uint32_t imageIndex = 0; imageIndex < imageCount; ++imageIndex )
    {
        basist::basisu_image_info imageInfo;
        transcoder.get_image_info( data, dataSizeInBytes, imageInfo, imageIndex );

        LOG( "Image index: ", imageIndex );
        LOG( "\tm_orig_width: ", imageInfo.m_orig_width );
        LOG( "\tm_orig_height: ", imageInfo.m_orig_height );
        LOG( "\tm_width: ", imageInfo.m_width );
        LOG( "\tm_height: ", imageInfo.m_height );
        LOG( "\tm_total_levels: ", imageInfo.m_total_levels );

        for ( uint32_t mipLevel = 0; mipLevel < imageInfo.m_total_levels; ++mipLevel )
        {
            basist::basisu_image_level_info levelInfo;
            transcoder.get_image_level_info( data, dataSizeInBytes, levelInfo, imageIndex, mipLevel );
            LOG( "\tMipLevel: ", mipLevel );
            LOG( "\t\tm_orig_width: ", levelInfo.m_orig_width );
            LOG( "\t\tm_orig_height: ", levelInfo.m_orig_height );
            LOG( "\t\tm_width: ", levelInfo.m_width );
            LOG( "\t\tm_height: ", levelInfo.m_height );
            LOG( "\t\tm_num_blocks_x: ", levelInfo.m_num_blocks_x );
            LOG( "\t\tm_num_blocks_y: ", levelInfo.m_num_blocks_y );
            LOG( "\t\tm_total_blocks: ", levelInfo.m_total_blocks );
            uint32_t levelSize = levelInfo.m_total_blocks * dstTexFormatSize;

            bool didTranscode = transcoder.transcode_image_level( data, dataSizeInBytes, imageIndex, mipLevel, currentSlice, levelSize, dstBasisTexFormat );
            if( !didTranscode )
            {
                LOG_ERR( "Could not transcode image: ", imageIndex, ", mip: ", mipLevel );
                return false;
            }
            currentSlice += levelSize;
        }
    }

    tex = Gfx::g_renderState.device.NewTextureCompressed( imageDesc, allTranscodedData );
    free( allTranscodedData );

    return true;
}

int main( int argc, char* argv[] )
{
    if ( argc != 2 )
    {
        std::cout << "Usage: ./example [path to scene file]" << std::endl;
        return 0;
    }

    if ( !PG::EngineInitialize() )
    {
        std::cout << "Failed to initialize the engine" << std::endl;
        return 0;
    }

    {
        Window* window = GetMainWindow();
        window->SetRelativeMouse( true );
    
        Scene* scene = Scene::Load( argv[1] );
        if ( !scene )
        {
            LOG_ERR( "Could not load scene '", argv[1], "'" );
            PG::EngineQuit();
            return 0;
        }

        scene->Start();

        //ImageCreateInfo imageCreateInfo;
        //imageCreateInfo.flags = IMAGE_CREATE_TEXTURE_ON_LOAD | IMAGE_GENERATE_MIPMAPS | IMAGE_FLIP_VERTICALLY | IMAGE_FREE_CPU_COPY_ON_LOAD;
        //imageCreateInfo.filename = PG_RESOURCE_DIR "textures/wood.png";
        //auto replacementImage = Image::Load2DImageWithDefaultSettings( PG_RESOURCE_DIR "textures/wood.png" );
        auto floorImage = ResourceManager::Get< Image >( "floor" );
        PG_ASSERT( floorImage );
        //*floorImage->GetTexture() = *replacementImage->GetTexture();


        if ( !TranscodeBasisFile( PG_ROOT_DIR "wood.basis", Gfx::PixelFormat::BC7_UNORM, *floorImage->GetTexture() ) )
        {
            LOG_ERR( "Could not transcode basis file" );
            delete scene;
            PG::EngineQuit();
            return 0;
        }


        PG::Input::PollEvents();
        Time::Reset();

        // Game loop
        while ( !PG::g_engineShutdown )
        {
            window->StartFrame();
            PG::Input::PollEvents();

            if ( PG::Input::GetKeyDown( PG::Key::ESC ) )
            {
                PG::g_engineShutdown = true;
            }
            if ( Input::GetKeyUp( Key::F1 ) )
            {
                Gfx::UIOverlay::SetVisible( !Gfx::UIOverlay::Visible() );
            }
            if ( PG::Input::GetKeyUp( PG::Key::P ) )
            {
                g_paused = !g_paused;
            }


            if ( !g_paused )
            {
                scene->Update();
            }
            RenderSystem::Render( scene );

            // std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );

            window->EndFrame();
        }

        PG::Gfx::g_renderState.device.WaitForIdle();
        delete scene;
    }

    PG::EngineQuit();

    return 0;
}
