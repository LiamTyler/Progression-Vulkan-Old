#include "resource/shader.hpp"
#include "graphics/pg_to_vulkan_types.hpp"
#include "graphics/vulkan.hpp"
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
            LOG_ERR( "Could not open shader file: '", createInfo->filename, "'" );
            return false;
        }

        uint32_t* spirvData = reinterpret_cast< uint32_t* >( memMappedFile.getData() );
        size_t spirvSize    = memMappedFile.size();

        VkShaderModuleCreateInfo vkShaderInfo = {};
        vkShaderInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        vkShaderInfo.codeSize = spirvSize;
        vkShaderInfo.pCode    = spirvData;

        VkResult ret = vkCreateShaderModule( Gfx::g_renderState.device.GetNativeHandle(), &vkShaderInfo, nullptr, &m_shaderModule );
        if ( ret != VK_SUCCESS )
        {
            return false;
        }

        reflectInfo = Reflect( spirvData, spirvSize );

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
        serialize::Read( buffer, reflectInfo.entryPoint );
        serialize::Read( buffer, reflectInfo.stage );
        size_t spirvSize;
        serialize::Read( buffer, spirvSize );

        VkShaderModuleCreateInfo vkShaderInfo = {};
        vkShaderInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        vkShaderInfo.codeSize = spirvSize;
        vkShaderInfo.pCode    = reinterpret_cast< const uint32_t* >( buffer );
        VkResult ret = vkCreateShaderModule( Gfx::g_renderState.device.GetNativeHandle(), &vkShaderInfo, nullptr, &m_shaderModule );
        if ( ret != VK_SUCCESS )
        {
            return false;
        }
        buffer += spirvSize;

        return true;
    }

    ShaderReflectInfo Shader::Reflect( const uint32_t* spirv, size_t spirvSizeInBytes )
    {
        ShaderReflectInfo info;

        SpvReflectShaderModule module;
        SpvReflectResult result = spvReflectCreateShaderModule( spirvSizeInBytes, spirv, &module );
        PG_MAYBE_UNUSED( result );
        PG_ASSERT( result == SPV_REFLECT_RESULT_SUCCESS );

        uint32_t var_count = 0;
        result = spvReflectEnumerateInputVariables( &module, &var_count, NULL );
        PG_ASSERT( result == SPV_REFLECT_RESULT_SUCCESS );
        SpvReflectInterfaceVariable* input_vars = static_cast< SpvReflectInterfaceVariable* >( malloc( var_count * sizeof( SpvReflectInterfaceVariable ) ) );
        result = spvReflectEnumerateInputVariables( &module, &var_count, &input_vars );
        PG_ASSERT( result == SPV_REFLECT_RESULT_SUCCESS );

        // LOG( "inputs = ", var_count );
        // for ( uint32_t i = 0; i < var_count; ++i )
        // {
        //     LOG( "input_var[", i, "] = ", input_vars[i].name, ", loc = ", input_vars[i].location );
        // }

        // LOG( "Entry point = ", module.entry_point_name );
        // LOG( "Entry point count = ", module.entry_point_count );
        // LOG( "Stage = ", module.shader_stage );
        // LOG( "spirv size = ", spirvSizeInBytes );
        PG_ASSERT( module.entry_point_count == 1);

        info.entryPoint = module.entry_point_name;
        info.stage      = static_cast< Gfx::ShaderStage >( module.shader_stage );

        spvReflectDestroyShaderModule( &module );

        return info;
    }

    VkPipelineShaderStageCreateInfo Shader::GetVkPipelineShaderStageCreateInfo() const
    {
        VkPipelineShaderStageCreateInfo info = {};
        info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        info.stage  = PGToVulkanShaderStage( reflectInfo.stage );
        info.module = m_shaderModule;
        info.pName  = reflectInfo.entryPoint.c_str();

        return info;
    }

    void Shader::Free()
    {
        if ( m_shaderModule != VK_NULL_HANDLE )
        {
            vkDestroyShaderModule( Gfx::g_renderState.device.GetNativeHandle(), m_shaderModule, nullptr );
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
