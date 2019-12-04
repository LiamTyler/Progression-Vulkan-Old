#include "resource/shader.hpp"
#include "graphics/debug_marker.hpp"
#include "graphics/pg_to_vulkan_types.hpp"
#include "graphics/vulkan.hpp"
#include "memory_map/MemoryMapped.h"
#include "resource/resource_manager.hpp"
#include "utils/fileIO.hpp"
#include "utils/logger.hpp"
#include "utils/serialize.hpp"
#include "SPIRV-Reflect/spirv_reflect.h"
#include "SPIRV-Reflect/common/output_stream.h"
#include <algorithm>
#include <sstream>

/*
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
*/

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
        name = createInfo->name;

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

        VkResult ret = vkCreateShaderModule( Gfx::g_renderState.device.GetHandle(), &vkShaderInfo, nullptr, &m_shaderModule );
        if ( ret != VK_SUCCESS )
        {
            return false;
        }
        PG_DEBUG_MARKER_SET_SHADER_NAME( (*this), name );

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
        size_t mapSize;
        serialize::Read( buffer, mapSize );
        for ( size_t i = 0; i < mapSize; ++i )
        {
            std::string varName;
            uint32_t location;
            serialize::Read( buffer, varName );
            serialize::Read( buffer, location );
            reflectInfo.inputLocations[varName] = location;
        }
        serialize::Read( buffer, mapSize );
        for ( size_t i = 0; i < mapSize; ++i )
        {
            std::string varName;
            uint32_t location;
            serialize::Read( buffer, varName );
            serialize::Read( buffer, location );
            reflectInfo.outputLocations[varName] = location;
        }

        size_t numDescriptorSets;
        serialize::Read( buffer, numDescriptorSets );
        reflectInfo.descriptorSetLayouts.resize( numDescriptorSets );
        for ( size_t i = 0; i < numDescriptorSets; ++i )
        {
            serialize::Read( buffer, reflectInfo.descriptorSetLayouts[i].setNumber );
            serialize::Read( buffer, reflectInfo.descriptorSetLayouts[i].createInfo );
            serialize::Read( buffer, reflectInfo.descriptorSetLayouts[i].bindings );

            reflectInfo.descriptorSetLayouts[i].createInfo.pBindings = reflectInfo.descriptorSetLayouts[i].bindings.data();
        }
        
        serialize::Read( buffer, reflectInfo.pushConstants );

        size_t spirvSize;
        serialize::Read( buffer, spirvSize );

        VkShaderModuleCreateInfo vkShaderInfo = {};
        vkShaderInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        vkShaderInfo.codeSize = spirvSize;
        vkShaderInfo.pCode    = reinterpret_cast< const uint32_t* >( buffer );
        VkResult ret = vkCreateShaderModule( Gfx::g_renderState.device.GetHandle(), &vkShaderInfo, nullptr, &m_shaderModule );
        if ( ret != VK_SUCCESS )
        {
            return false;
        }
        PG_DEBUG_MARKER_SET_SHADER_NAME( (*this), name );
        buffer += spirvSize;

        return true;
    }

#if USING( DEBUG_BUILD )
    // From SPIRV-Reflect
    static void PrintDescriptorBinding( const SpvReflectDescriptorBinding& obj, bool write_set, const char* indent )
    {
        const char* t = indent;
        LOG( t, "binding : ", obj.binding );
        if ( write_set )
        {
            LOG( t, "set     : ", obj.set );
        }
        LOG( t, "type    : ", ToStringDescriptorType( obj.descriptor_type ) );

        // array
        if ( obj.array.dims_count > 0 )
        {
            std::stringstream os;
            os << t << "array   : ";
            for ( uint32_t dim_index = 0; dim_index < obj.array.dims_count; ++dim_index )
            {
                os << "[" << obj.array.dims[dim_index] << "]";
            }
            LOG( os.str() );
        }

        // counter
        if ( obj.uav_counter_binding != nullptr )
        {
            LOG( t, "counter : (set = ", obj.uav_counter_binding->set, ", binding = ", obj.uav_counter_binding->binding, ", name = ", obj.uav_counter_binding->name, ");" );
        }

        if ( ( obj.type_description->type_name != nullptr ) && ( strlen(obj.type_description->type_name ) > 0 ) )
        {
            LOG( t, "name    : ", obj.name, " (", obj.type_description->type_name, ")" );
        }
        else
        {
            LOG( t, "name    : ", obj.name );
        }
    }

    // From SPIRV-Reflect
    static void PrintDescriptorSet( const SpvReflectDescriptorSet& obj, const char* indent )
    {
        const char* t     = indent;
        std::string tt    = std::string(indent) + "  ";
        std::string ttttt = std::string(indent) + "    ";

        LOG( t, "set           : ", obj.set );
        LOG( t, "binding count : ", obj.binding_count );
        for ( uint32_t i = 0; i < obj.binding_count; ++i )
        {
            const SpvReflectDescriptorBinding& binding = *obj.bindings[i];
            LOG( tt, i, ":" );
            PrintDescriptorBinding( binding, false, ttttt.c_str() );
            if ( i < ( obj.binding_count - 1 ) )
            {
                LOG( "" );
            }
        }
    }
#endif // #if USING( DEBUG_BUILD )

    static void PrintSpvBlockVariable( SpvReflectBlockVariable* var, const std::string& indent = "" )
    {
        LOG( indent + "Variable = ", var->name, ", size = ", var->size, ", offset = ", var->offset, ", abs offset = ", var->absolute_offset );
        for ( uint32_t m = 0; m < var->member_count; ++m )
        {
            PrintSpvBlockVariable( &var->members[m], indent + "\t" );
        }
    }

    ShaderReflectInfo Shader::Reflect( const uint32_t* spirv, size_t spirvSizeInBytes )
    {
        ShaderReflectInfo info;

        SpvReflectShaderModule module;
        SpvReflectResult result = spvReflectCreateShaderModule( spirvSizeInBytes, spirv, &module );
        PG_MAYBE_UNUSED( result );
        PG_ASSERT( result == SPV_REFLECT_RESULT_SUCCESS );
        PG_ASSERT( module.entry_point_count == 1 );

        uint32_t count = 0;
        result = spvReflectEnumerateInputVariables( &module, &count, NULL );
        PG_ASSERT( result == SPV_REFLECT_RESULT_SUCCESS );

        std::vector< SpvReflectInterfaceVariable* > inputVars( count );
        result = spvReflectEnumerateInputVariables( &module, &count, inputVars.data() );
        PG_ASSERT( result == SPV_REFLECT_RESULT_SUCCESS );

        count = 0;
        result = spvReflectEnumerateOutputVariables( &module, &count, NULL );
        PG_ASSERT( result == SPV_REFLECT_RESULT_SUCCESS );

        std::vector< SpvReflectInterfaceVariable* > outputVars( count );
        result = spvReflectEnumerateOutputVariables( &module, &count, outputVars.data() );
        PG_ASSERT( result == SPV_REFLECT_RESULT_SUCCESS );

        LOG( "inputs = ", inputVars.size() );
        for ( size_t i = 0; i < inputVars.size(); ++i )
        {
            LOG( "inputVars[", i, "] = ", inputVars[i]->name, ", loc = ", inputVars[i]->location );
            info.inputLocations[inputVars[i]->name] = inputVars[i]->location;
        }

        LOG( "outputs = ", outputVars.size() );
        for ( size_t i = 0; i < outputVars.size(); ++i )
        {
            LOG( "outputVars[", i, "] = ", outputVars[i]->name, ", loc = ", outputVars[i]->location );
            info.outputLocations[outputVars[i]->name] = outputVars[i]->location;
        }

        info.entryPoint = module.entry_point_name;
        info.stage      = static_cast< Gfx::ShaderStage >( module.shader_stage );
        std::unordered_map< Gfx::ShaderStage, std::string > shaderStageNames =
        {
            { Gfx::ShaderStage::VERTEX, "VERTEX" },
            { Gfx::ShaderStage::TESSELLATION_CONTROL, "TESSELLATION_CONTROL" },
            { Gfx::ShaderStage::TESSELLATION_EVALUATION, "TESSELLATION_EVALUATION" },
            { Gfx::ShaderStage::GEOMETRY, "GEOMETRY" },
            { Gfx::ShaderStage::FRAGMENT, "FRAGMENT" },
            { Gfx::ShaderStage::COMPUTE, "COMPUTE" },
        };

        LOG( "Entry point       = ", module.entry_point_name );
        LOG( "Stage             = ", shaderStageNames[info.stage] );
        LOG( "Source lang       = ", spvReflectSourceLanguage( module.source_language ) );
        LOG( "Source lang ver   = ", module.source_language_version );
        LOG( "Spirv size        = ", spirvSizeInBytes );

        result = spvReflectEnumeratePushConstantBlocks( &module, &count, NULL );
        PG_ASSERT( result == SPV_REFLECT_RESULT_SUCCESS );

        std::vector< SpvReflectBlockVariable* > pushConstants( count );
        result = spvReflectEnumeratePushConstantBlocks( &module, &count, pushConstants.data() );
        PG_ASSERT( result == SPV_REFLECT_RESULT_SUCCESS );
        for ( size_t i = 0; i < pushConstants.size(); ++i )
        {
            LOG( "Push constant[", i, "]: " );
            PrintSpvBlockVariable( pushConstants[i], "\t" );
        }

        for ( size_t i = 0; i < pushConstants.size(); ++i )
        {
            for ( uint32_t m = 0; m < pushConstants[i]->member_count; ++m )
            {
                VkPushConstantRange r;
                r.stageFlags = static_cast< VkShaderStageFlagBits >( module.shader_stage );
                r.offset     = pushConstants[i]->members[m].absolute_offset;
                r.size       = pushConstants[i]->members[m].size;
                info.pushConstants.push_back( r );
            }
        }

        std::sort( info.pushConstants.begin(), info.pushConstants.end(), []( const auto& lhs, const auto& rhs ) { return lhs.offset < rhs.offset; } );
        for ( size_t i = 1; i < info.pushConstants.size(); ++i )
        {
            if ( info.pushConstants[i].offset == info.pushConstants[i-1].offset + info.pushConstants[i-1].size )
            {
                info.pushConstants[i-1].size += info.pushConstants[i].size;
                info.pushConstants[i-1].stageFlags |= info.pushConstants[i].stageFlags;
                info.pushConstants.erase( info.pushConstants.begin() + i );
                --i;
            }
        }

        uint32_t maxPushConstantSize = Gfx::g_renderState.physicalDeviceInfo.deviceProperties.limits.maxPushConstantsSize;
        for ( size_t i = 0; i < info.pushConstants.size(); ++i )
        {
            LOG( "Range[", i, "] offset = ", info.pushConstants[i].offset, ", size = ", info.pushConstants[i].size );
            PG_ASSERT( info.pushConstants[i].offset + info.pushConstants[i].size <= maxPushConstantSize,
                "The push constants specified in this shader exceed the size limit for this gpu: " + std::to_string( maxPushConstantSize ) );
        }

        result = spvReflectEnumerateDescriptorSets( &module, &count, NULL );
        PG_ASSERT( result == SPV_REFLECT_RESULT_SUCCESS );

        std::vector< SpvReflectDescriptorSet* > sets( count );
        result = spvReflectEnumerateDescriptorSets( &module, &count, sets.data() );
        PG_ASSERT( result == SPV_REFLECT_RESULT_SUCCESS );

        info.descriptorSetLayouts.resize( sets.size(), Gfx::DescriptorSetLayoutData{} );
        for ( size_t i_set = 0; i_set < sets.size(); ++i_set )
        {
            const SpvReflectDescriptorSet& refl_set = *(sets[i_set]);
            Gfx::DescriptorSetLayoutData& layout         = info.descriptorSetLayouts[i_set];
            layout.bindings.resize( refl_set.binding_count );
            for ( uint32_t i_binding = 0; i_binding < refl_set.binding_count; ++i_binding )
            {
                const SpvReflectDescriptorBinding& refl_binding = *(refl_set.bindings[i_binding]);
                VkDescriptorSetLayoutBinding& layout_binding    = layout.bindings[i_binding];
                layout_binding.binding                          = refl_binding.binding;
                layout_binding.descriptorType                   = static_cast< VkDescriptorType >( refl_binding.descriptor_type );
                layout_binding.descriptorCount                  = 1;
                for ( uint32_t i_dim = 0; i_dim < refl_binding.array.dims_count; ++i_dim )
                {
                    layout_binding.descriptorCount *= refl_binding.array.dims[i_dim];
                }
                layout_binding.stageFlags         = static_cast<VkShaderStageFlagBits >( module.shader_stage );
                layout_binding.pImmutableSamplers = nullptr;
            }

            layout.setNumber               = refl_set.set;
            layout.createInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layout.createInfo.bindingCount = refl_set.binding_count;
            layout.createInfo.pBindings    = layout.bindings.data();
        }

        std::sort( info.descriptorSetLayouts.begin(), info.descriptorSetLayouts.end(),
                []( const auto& lhs, const auto& rhs ) { return lhs.setNumber < rhs.setNumber; } );
        for ( size_t i_set = 0; i_set < sets.size(); ++i_set )
        {
            info.descriptorSetLayouts[i_set].createInfo.pBindings = info.descriptorSetLayouts[i_set].bindings.data();
        }

#if USING( DEBUG_BUILD )
        const char* t  = "  ";
        const char* tt = "    ";
        LOG( "Descriptor sets:" );
        for ( size_t index = 0; index < sets.size(); ++index )
        {
            auto p_set = sets[index];

            // descriptor sets can also be retrieved directly from the module, by set index
            auto p_set2 = spvReflectGetDescriptorSet( &module, p_set->set, &result );
            PG_ASSERT( result == SPV_REFLECT_RESULT_SUCCESS );
            PG_ASSERT( p_set == p_set2 );
            PG_UNUSED( p_set2 );

            LOG( t, index, ":" );
            PrintDescriptorSet( *p_set, tt );
            LOG( "\n" );
        }
#endif // #if USING( DEBUG_BUILD )

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
            vkDestroyShaderModule( Gfx::g_renderState.device.GetHandle(), m_shaderModule, nullptr );
            m_shaderModule = VK_NULL_HANDLE;
        }
    }

    VkShaderModule Shader::GetHandle() const
    {
        return m_shaderModule;
    }

    Shader::operator bool() const
    {
        return m_shaderModule != VK_NULL_HANDLE;
    }

} // namespace Progression
