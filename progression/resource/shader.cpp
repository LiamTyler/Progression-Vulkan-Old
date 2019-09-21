#include "resource/shader.hpp"
// #include "graphics/graphics_api.hpp"
#include "memory_map/MemoryMapped.h"
#include "resource/resource_manager.hpp"
#include "utils/fileIO.hpp"
#include "utils/logger.hpp"
#include "utils/serialize.hpp"
#include "SPIRV-Reflect/spirv_reflect.h"

static bool LoadBinaryFile( std::string& source, const std::string& filename )
{
    std::ifstream in( filename, std::ios::binary );
    if ( !in.is_open() )
    {
        LOG_ERR( "Failed to open the file: ", filename );
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
        m_shaderModule( VK_NULL_HANDLE )
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

        shader.m_shaderModule = VK_NULL_HANDLE;

        return *this;
    }

    bool Shader::Load( ResourceCreateInfo* info )
    {
        ShaderCreateInfo* createInfo = static_cast< ShaderCreateInfo* >( info );
        Free();

        MemoryMapped memMappedFile;
        if ( !memMappedFile.open( createInfo->filename, MemoryMapped::WholeFile, MemoryMapped::SequentialScan ) )
        {
            LOG_ERR( "Could not open fastfile:", createInfo->filename );
            return false;
        }

        uint32_t* spirvData = reinterpret_cast< uint32_t* >( memMappedFile.getData() );
        size_t spirvSize    = memMappedFile.size();

        VkShaderModuleCreateInfo vkShaderInfo = {};
        vkShaderInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        vkShaderInfo.codeSize = spirvSize;
        vkShaderInfo.pCode    = spirvData;

        VkResult ret = vkCreateShaderModule( Gfx::g_device.GetNativeHandle(), &vkShaderInfo, nullptr, &m_shaderModule );
        if ( ret != VK_SUCCESS )
        {
            return false;
        }

        SpvReflectShaderModule module;
        SpvReflectResult result = spvReflectCreateShaderModule( spirvSize, spirvData, &module);
        assert(result == SPV_REFLECT_RESULT_SUCCESS);

        // Enumerate and extract shader's input variables
        uint32_t var_count = 0;
        result = spvReflectEnumerateInputVariables(&module, &var_count, NULL);
        assert(result == SPV_REFLECT_RESULT_SUCCESS);
        SpvReflectInterfaceVariable* input_vars =
        (SpvReflectInterfaceVariable*)malloc(var_count * sizeof(SpvReflectInterfaceVariable));
        result = spvReflectEnumerateInputVariables(&module, &var_count, &input_vars);
        assert(result == SPV_REFLECT_RESULT_SUCCESS);
        PG_UNUSED( result );

        // Output variables, descriptor bindings, descriptor sets, and push constants
        // can be enumerated and extracted using a similar mechanism.
        LOG( "inputs = ", var_count );
        for ( uint32_t i = 0; i < var_count; ++i )
        {
            LOG( "input_var[", i, "]= ", input_vars[i].name );
        }

        // Destroy the reflection data when no longer required.
        spvReflectDestroyShaderModule(&module);


        return true;
    }

    void Shader::Move( std::shared_ptr< Resource > dst )
    {
        PG_ASSERT( std::dynamic_pointer_cast< Shader >( dst ) );
        Shader* dstPtr = (Shader*) dst.get();
        *dstPtr        = std::move( *this );
    }

    bool Shader::Serialize( std::ofstream& out ) const
    {
        // The shader serialization is done in shader_converter.cpp
        PG_UNUSED( out );
        return false;
    }

    bool Shader::Deserialize( char*& buffer )
    {
        serialize::Read( buffer, name );

        return false;
    }

    void Shader::Free()
    {
        if ( m_shaderModule != VK_NULL_HANDLE )
        {
            vkDestroyShaderModule( Gfx::g_device.GetNativeHandle(), m_shaderModule, nullptr );
            m_shaderModule = VK_NULL_HANDLE;
        }
    }

    VkShaderModule Shader::GetNativeHandle() const
    {
        return m_shaderModule;
    }

    Shader::operator bool() const
    {
        return m_shaderModule != VK_NULL_HANDLE;
    }

} // namespace Progression
