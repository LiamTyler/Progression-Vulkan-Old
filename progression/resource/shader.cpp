#include "resource/shader.hpp"
#include "resource/resource_manager.hpp"
#include "utils/fileIO.hpp"
#include "utils/logger.hpp"
#include "utils/serialize.hpp"

static bool LoadShaderTextFile( std::string& source, const std::string& filename )
{
    std::ifstream in( filename );
    if ( !in.is_open() )
    {
        LOG_ERR( "Failed to open the shader file: ", filename );
        return false;
    }

    in.seekg( 0, std::ios::end );
    size_t size = in.tellg();
    source.resize( size );
    in.seekg( 0 );
    in.read( &source[0], size );

    return true;
}

namespace Progression
{

Shader::Shader() :
    Resource( "" ),
    m_loaded( false )
{
}

Shader::~Shader()
{
    Free();
}

Shader::Shader( Shader&& shader )
{
    *this = std::move( shader );
}

Shader& Shader::operator=( Shader&& shader )
{
    name           = std::move( shader.name );
    m_shaderModule = shader.m_shaderModule;
    m_loaded       = shader.m_loaded;

    shader.m_loaded = false;

    return *this;
}

bool Shader::Load( ResourceCreateInfo* info )
{
    ShaderCreateInfo* createInfo = static_cast< ShaderCreateInfo* >( info );
    Free();

    return false;
}

void Shader::Move( std::shared_ptr< Resource > dst )
{
    PG_ASSERT( std::dynamic_pointer_cast< Shader >( dst ) );
    Shader* dstPtr = (Shader*) dst.get();
    *dstPtr        = std::move( *this );
}

bool Shader::Serialize( std::ofstream& out ) const
{
    serialize::Write( out, name );

    return false;
    // return !out.fail();
}

bool Shader::Deserialize( char*& buffer )
{
    serialize::Read( buffer, name );

    return false;
}

void Shader::Free()
{
    if ( m_loaded )
    {
        // vkDestroyShaderModule( logicalDevice, m_shaderModule, nullptr );
        m_loaded = false;
    }
}

VkShaderModule Shader::GetNativeHandle() const
{
    return m_shaderModule;
}

Shader::operator bool() const
{
    return m_loaded;
}

} // namespace Progression
