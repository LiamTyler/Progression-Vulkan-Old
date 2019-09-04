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

static Gfx::PixelFormat GetSourceFormatFromNumComponents( int numComponents )
{
    Gfx::PixelFormat srcFormat[] = {
        Gfx::PixelFormat::R8_Uint,
        Gfx::PixelFormat::R8_G8_Uint,
        Gfx::PixelFormat::R8_G8_B8_Uint,
        Gfx::PixelFormat::R8_G8_B8_A8_Uint,
    };

    return srcFormat[numComponents - 1];
}

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
    info->texDesc.height = img.Height();
    info->texDesc.width  = img.Width();

    gfxTexture = Gfx::Texture::Create( info->texDesc, img.Pixels(),
                                       GetSourceFormatFromNumComponents( img.NumComponents() ) );
    sampler    = RenderSystem::GetSampler( &info->samplerDesc );

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

    return true;
}

GLuint Texture::GetNativeHandle() const
{
    return gfxTexture.GetNativeHandle();
}

Gfx::PixelFormat Texture::GetFormat() const
{
    return gfxTexture.GetFormat();
}

Gfx::TextureType Texture::GetType() const
{
    return gfxTexture.GetType();
}

uint32_t Texture::Width() const
{
    return gfxTexture.GetWidth();
}

uint32_t Texture::Height() const
{
    return gfxTexture.GetHeight();
}

bool Texture::GetMipMapped() const
{
    return gfxTexture.GetMipMapped();
}

} // namespace Progression
