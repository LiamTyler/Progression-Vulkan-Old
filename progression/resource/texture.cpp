#include "resource/texture.hpp"
#include "core/assert.hpp"
#include "graphics/render_system.hpp"
#include "resource/image.hpp"
#include "resource/resource_manager.hpp"
#include "utils/fileIO.hpp"
#include "utils/logger.hpp"
#include "utils/serialize.hpp"

namespace Progression
{

Texture::Texture() : Resource( "" )
{
}

Texture::Texture( Texture&& texture )
{
    *this = std::move( texture );
}

Texture& Texture::operator=( Texture&& texture )
{
    name       = std::move( texture.name );
    gfxTexture = std::move( texture.gfxTexture );
    sampler    = std::move( texture.sampler );

    return *this;
}

bool Texture::Load( ResourceCreateInfo* createInfo )
{
    PG_ASSERT( createInfo );
    TextureCreateInfo* info = static_cast< TextureCreateInfo* >( createInfo );
    name                    = info->name;

    Image img;
    if ( !img.Load( info->filename ) )
    {
        LOG_ERR( "Could not load texture image: ", info->filename );
        return false;
    }
    info->imageDesc = img.GetDescriptor();

    gfxTexture = Gfx::Texture::Create( info->imageDesc, img.GetPixels() );
    // sampler    = RenderSystem::GetSampler( &info->samplerDesc );

    return true;
}

void Texture::Move( std::shared_ptr< Resource > dst )
{
    PG_ASSERT( std::dynamic_pointer_cast< Texture >( dst ) );
    Texture* dstPtr = (Texture*) dst.get();
    *dstPtr         = std::move( *this );
}

bool Texture::Serialize( std::ofstream& out ) const
{
    PG_UNUSED( out );
    // Serialization done in TextureConverter
    return false;
}

bool Texture::Deserialize( char*& buffer )
{
    // serialize::Read( buffer, m_desc.type );
    // serialize::Read( buffer, m_desc.format );
    // serialize::Read( buffer, m_desc.mipLevels );
    // serialize::Read( buffer, m_desc.arrayLayers  );
    // serialize::Read( buffer, m_desc.width );
    // serialize::Read( buffer, m_desc.height );
    // serialize::Read( buffer, m_desc.depth );
    // size_t totalSize;
    // serialize::Read( buffer, totalSize );
    // m_pixels = (unsigned char*) malloc( totalSize );
    // serialize::Read( buffer, (char*) m_pixels, totalSize );
    /*
    Gfx::TextureDescriptor texDesc;
    serialize::Read( buffer, name );
    serialize::Read( buffer, texDesc.format );
    serialize::Read( buffer, texDesc.type );
    serialize::Read( buffer, texDesc.mipmapped );

    Gfx::SamplerDescriptor samplerDesc;
    serialize::Read( buffer, samplerDesc.minFilter );
    serialize::Read( buffer, samplerDesc.magFilter );
    serialize::Read( buffer, samplerDesc.wrapModeS );
    serialize::Read( buffer, samplerDesc.wrapModeT );
    serialize::Read( buffer, samplerDesc.wrapModeR );
    serialize::Read( buffer, samplerDesc.borderColor );
    serialize::Read( buffer, samplerDesc.maxAnisotropy );
    sampler = RenderSystem::GetSampler( &samplerDesc );

    serialize::Read( buffer, texDesc.width );
    serialize::Read( buffer, texDesc.height );
    uint32_t numComponents;
    serialize::Read( buffer, numComponents );
    gfxTexture = Gfx::Texture::Create( texDesc, buffer, GetSourceFormatFromNumComponents( numComponents ) );
    buffer += texDesc.width * texDesc.height * numComponents;
    */
    PG_UNUSED( buffer );

    return true;
}

} // namespace Progression
