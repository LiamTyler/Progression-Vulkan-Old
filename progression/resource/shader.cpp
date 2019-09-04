#include "resource/shader.hpp"
#include "graphics/pg_to_opengl_types.hpp"
#include "resource/resource_manager.hpp"
#include "utils/fileIO.hpp"
#include "utils/logger.hpp"
#include "utils/serialize.hpp"


// TODO: faster read file
static bool LoadShaderTextFile( std::string& source, const std::string& filename )
{
    std::ifstream in( filename );
    if ( in.fail() )
    {
        LOG_ERR( "Failed to open the shader file: ", filename );
        return false;
    }
    source = "";
    std::string line;
    while ( std::getline( in, line ) ) source += line + '\n';
    in.close();
    return true;
}

static bool CompileShader( GLuint& shader, const char* source, GLenum shaderType )
{
    shader = glCreateShader( shaderType );
    glShaderSource( shader, 1, &source, NULL );
    glCompileShader( shader );

    GLint result = GL_FALSE;
    int infoLogLength;

    glGetShaderiv( shader, GL_COMPILE_STATUS, &result );
    glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &infoLogLength );
    if ( !result )
    {
        std::vector< char > errorMessage( infoLogLength + 1 );
        glGetShaderInfoLog( shader, infoLogLength, NULL, &errorMessage[0] );
        std::string err( &errorMessage[0] );
        LOG_ERR( "Error while loading shader:\n", err, '\n' );
        glDeleteShader( shader );
        return false;
    }

    return true;
}

static bool CreateAndLinkProgram( GLuint& program, const std::vector< GLuint >& shaders )
{
    program = glCreateProgram();
    for ( const auto& shader : shaders ) glAttachShader( program, shader );

    glLinkProgram( program );

    for ( const auto& shader : shaders )
    {
        glDetachShader( program, shader );
        glDeleteShader( shader );
    }

    GLint result = GL_FALSE;
    int infoLogLength;
    glGetProgramiv( program, GL_LINK_STATUS, &result );
    glGetProgramiv( program, GL_INFO_LOG_LENGTH, &infoLogLength );
    if ( !result )
    {
        std::vector< char > errorMessage( infoLogLength + 1 );
        glGetProgramInfoLog( program, infoLogLength, NULL, &errorMessage[0] );
        std::string err( &errorMessage[0] );
        LOG_ERR( "Error while compiling and linking the shader:\n", err, '\n' );
        glDeleteProgram( program );
        return false;
    }

    return true;
}

namespace Progression
{

Shader::Shader() : Resource( "" ), m_program( ~0u )
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
    name       = std::move( shader.name );
    m_program  = std::move( shader.m_program );
    m_uniforms = std::move( shader.m_uniforms );

    shader.m_program = (GLuint) -1;

    return *this;
}

bool Shader::Load( ResourceCreateInfo* info )
{
    ShaderCreateInfo* createInfo = static_cast< ShaderCreateInfo* >( info );
#if USING( SERIALIZE_SHADER_TEXT )
    m_createInfo = *createInfo;
#endif // #if USING( SERIALIZE_SHADER_TEXT )
    Free();

    std::string shaderSource;
    GLuint glShader;
    std::vector< GLuint > shaders;
    GLuint program;
    bool ret = true;
    name     = info->name;

    if ( !createInfo->compute.empty() )
    {
        ret = LoadShaderTextFile( shaderSource, createInfo->compute );
        ret = ret && CompileShader( glShader, shaderSource.c_str(), GL_COMPUTE_SHADER );
        shaders.push_back( glShader );
        ret = ret && CreateAndLinkProgram( program, shaders );
        if ( !ret )
            return false;
        m_program = program;
        return true;
    }

    if ( !createInfo->vertex.empty() )
    {
        ret = ret && LoadShaderTextFile( shaderSource, createInfo->vertex );
        ret = ret && CompileShader( glShader, shaderSource.c_str(), GL_VERTEX_SHADER );
        if ( ret )
            shaders.push_back( glShader );
    }
    if ( !createInfo->geometry.empty() )
    {
        ret = ret && LoadShaderTextFile( shaderSource, createInfo->geometry );
        ret = ret && CompileShader( glShader, shaderSource.c_str(), GL_GEOMETRY_SHADER );
        if ( ret )
            shaders.push_back( glShader );
    }
    if ( !createInfo->fragment.empty() )
    {
        ret = ret && LoadShaderTextFile( shaderSource, createInfo->fragment );
        ret = ret && CompileShader( glShader, shaderSource.c_str(), GL_FRAGMENT_SHADER );
        if ( ret )
            shaders.push_back( glShader );
    }
    if ( !ret )
    {
        for ( const auto& s : shaders ) glDeleteShader( s );
        return false;
    }

    if ( CreateAndLinkProgram( program, shaders ) )
    {
        m_program = program;
        QueryUniforms();
        return true;
    }
    else
    {
        return false;
    }
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

#if USING( SERIALIZE_SHADER_TEXT )
    serialize::Write( out, m_createInfo.vertex );
    serialize::Write( out, m_createInfo.geometry );
    serialize::Write( out, m_createInfo.fragment );
    serialize::Write( out, m_createInfo.compute );
#else // #if USING( SERIALIZE_SHADER_TEXT )
    GLint len;
    GLenum format;
    auto binary = GetShaderBinary( len, format );
    if ( !binary.size() )
    {
        return false;
    }

    serialize::Write( out, len );
    serialize::Write( out, format );
    serialize::Write( out, binary.data(), len );

#endif // #else // #if USING( SERIALIZE_SHADER_TEXT )

    uint32_t numUniforms = (uint32_t) m_uniforms.size();
    serialize::Write( out, numUniforms );
    for ( const auto& [uName, loc] : m_uniforms )
    {
        serialize::Write( out, uName );
        serialize::Write( out, loc );
    }

    return !out.fail();
}

bool Shader::Deserialize( char*& buffer )
{
    serialize::Read( buffer, name );
#if USING( SERIALIZE_SHADER_TEXT )
    m_createInfo.name = name;
    serialize::Read( buffer, m_createInfo.vertex );
    serialize::Read( buffer, m_createInfo.geometry );
    serialize::Read( buffer, m_createInfo.fragment );
    serialize::Read( buffer, m_createInfo.compute );
    
    if( !Load( &m_createInfo ) )
    {
        return false;
    }
#else // #if USING( SERIALIZE_SHADER_TEXT )
    GLint len;
    GLenum format;
    serialize::Read( buffer, len );
    serialize::Read( buffer, format );
    std::vector< char > binary( len );
    serialize::Read( buffer, binary.data(), len );
    if ( !LoadFromBinary( binary.data(), len, format ) )
    {
        return false;
    }
#endif // #else // #if USING( SERIALIZE_SHADER_TEXT )

    uint32_t numUniforms;
    serialize::Read( buffer, numUniforms );
    for ( uint32_t i = 0; i < numUniforms; ++i )
    {
        std::string uName;
        GLuint loc;
        serialize::Read( buffer, uName );
        serialize::Read( buffer, loc );
        m_uniforms[uName] = loc;
    }

    return true;
}

bool Shader::LoadFromBinary( const char* binarySource, GLint len, GLenum format )
{
    Free();
    GLuint program = glCreateProgram();
    glProgramBinary( program, format, binarySource, len );

    GLint status;
    glGetProgramiv( program, GL_LINK_STATUS, &status );
    GLenum err;
    if ( GL_FALSE == status )
    {
        LOG_ERR( "Failed to create program from binary" );
        return false;
    }
    m_program = program;
    return true;
}

std::vector< char > Shader::GetShaderBinary( GLint& len, GLenum& format ) const
{
    GLint formats = 0;
    glGetIntegerv( GL_NUM_PROGRAM_BINARY_FORMATS, &formats );
    if ( formats < 1 )
    {
        LOG_ERR( "No binary formats supported with this driver" );
        return {};
    }

    glGetProgramiv( m_program, GL_PROGRAM_BINARY_LENGTH, &len );
    auto binary = std::vector< char >( len );
    glGetProgramBinary( m_program, len, NULL, &format, binary.data() );

    return binary;
}

void Shader::Free()
{
    if ( m_program != (GLuint) -1 )
    {
        glDeleteProgram( m_program );
        m_program = (GLuint) -1;
    }
}

void Shader::QueryUniforms()
{
    GLsizei uniformBufSize;
    GLint count;
    glGetProgramiv( m_program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &uniformBufSize );
    GLchar* uniformName = new GLchar[uniformBufSize];
    glGetProgramiv( m_program, GL_ACTIVE_UNIFORMS, &count );
    for ( int i = 0; i < count; i++ )
    {
        GLint size;
        GLenum type;
        GLsizei length;
        glGetActiveUniform( m_program, (GLuint) i, uniformBufSize, &length, &size, &type,
                            uniformName );

        std::string sName( uniformName );
        // fix uniform arrays
        auto location = glGetUniformLocation( m_program, sName.c_str() );
        int len       = static_cast< int >( sName.length() );
        if ( len > 3 )
        {
            if ( sName[len - 1] == ']' && sName[len - 3] == '[' )
            {
                m_uniforms[sName.substr( 0, len - 3 )] = location;
#if USING( DEBUG_BUILD )
                LOG( "Uniform: ", location, " = ", sName.substr( 0, len - 3 ) );
#endif
            }
        }
        m_uniforms[sName] = location;
#if USING( DEBUG_BUILD )
        LOG( "Uniform: ", location, " = '", sName, "'" );
#endif
    }
#if USING( DEBUG_BUILD )
    LOG( "" );
#endif
    delete[] uniformName;
}

void Shader::Enable() const
{
    glUseProgram( m_program );
}

void Shader::Disable() const
{
    glUseProgram( 0 );
}

GLuint Shader::GetUniform( const std::string& _name )
{
    auto it = m_uniforms.find( _name );
    if ( it == m_uniforms.end() )
    {
#if USING( DEBUG_BUILD )
        std::hash< std::string > hasher;
        auto hash = hasher( _name );
        if ( m_warnings.find( hash ) == m_warnings.end() )
        {
            LOG_WARN( "Uniform: ", _name, " is not present in the shader: ", m_program,
                      ", using -1 instead" );
            m_warnings.insert( hash );
        }
#endif // #if USING( DEBUG_BUILD )

        return (GLuint) -1;
    }
    else
    {
        return it->second;
    }
}

GLuint Shader::GetAttribute( const std::string& _name )
{
    GLuint loc = glGetAttribLocation( m_program, _name.c_str() );
#if USING( DEBUG_BUILD )
    if ( loc == (GLuint) -1 )
    {
        std::hash< std::string > hasher;
        size_t hash = hasher( _name );
        if ( m_warnings.find( hash ) == m_warnings.end() )
        {
            LOG_WARN( "Attribute: ", _name, " is not present in the shader, using -1 instead" );
            m_warnings.insert( hash );
        }
    }
#endif // #if USING( DEBUG_BUILD )

    return loc;
}

void Shader::BindTexture( const Gfx::Texture& tex, const std::string& texName, uint32_t index )
{

    glActiveTexture( GL_TEXTURE0 + index );
    auto nativeTexType = Gfx::PGToOpenGLTextureType( tex.GetType() );
    glBindTexture( nativeTexType, tex.GetNativeHandle() );
    SetUniform( texName, (int) index );
}

GLuint Shader::GetNativeHandle() const
{
    return m_program;
}

void Shader::SetUniform( const std::string& _name, const bool data )
{
    glUniform1i( GetUniform( _name ), data );
}

void Shader::SetUniform( const std::string& _name, const int data )
{
    glUniform1i( GetUniform( _name ), data );
}

void Shader::SetUniform( const std::string& _name, const float data )
{
    glUniform1f( GetUniform( _name ), data );
}

void Shader::SetUniform( const std::string& _name, const glm::ivec2& data )
{
    glUniform2i( GetUniform( _name ), data.x, data.y );
}

void Shader::SetUniform( const std::string& _name, const glm::vec2& data )
{
    glUniform2f( GetUniform( _name ), data.x, data.y );
}

void Shader::SetUniform( const std::string& _name, const glm::vec3& data )
{
    glUniform3f( GetUniform( _name ), data.x, data.y, data.z );
}

void Shader::SetUniform( const std::string& _name, const glm::vec4& data )
{
    glUniform4f( GetUniform( _name ), data.x, data.y, data.z, data.w );
}

void Shader::SetUniform( const std::string& _name, const glm::mat3& data )
{
    glUniformMatrix3fv( GetUniform( _name ), 1, GL_FALSE, glm::value_ptr( data ) );
}

void Shader::SetUniform( const std::string& _name, const glm::mat4& data )
{
    glUniformMatrix4fv( GetUniform( _name ), 1, GL_FALSE, glm::value_ptr( data ) );
}

void Shader::SetUniform( const std::string& _name, const glm::mat4* data, int elements )
{
    glUniformMatrix4fv( GetUniform( _name ), elements, GL_FALSE, glm::value_ptr( data[0] ) );
}

void Shader::SetUniform( const std::string& _name, const glm::vec3* data, int elements )
{
    glUniform3fv( GetUniform( _name ), elements, glm::value_ptr( data[0] ) );
}

void Shader::SetUniform( const std::string& _name, const glm::vec4* data, int elements )
{
    glUniform4fv( GetUniform( _name ), elements, glm::value_ptr( data[0] ) );
}

} // namespace Progression
