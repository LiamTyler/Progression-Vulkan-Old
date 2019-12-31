#include "graphics/graphics_api/ui.hpp"
#include "core/assert.hpp"
#include "core/input.hpp"
#include "core/time.hpp"
#include "core/window.hpp"
#include "graphics/graphics_api/buffer.hpp"
#include "graphics/graphics_api/descriptor.hpp"
#include "graphics/graphics_api/pipeline.hpp"
#include "graphics/graphics_api/texture.hpp"
#include "graphics/vulkan.hpp"
#include "resource/resource_manager.hpp"
#include "resource/shader.hpp"
#include "utils/logger.hpp"

namespace Progression
{
namespace Gfx
{

static Texture s_fontTexture;
static DescriptorPool s_descriptorPool;
static std::vector< DescriptorSetLayout > s_descriptorSetLayouts;
static DescriptorSet s_descriptorSet;
static Pipeline s_pipeline;
static Buffer s_VBO, s_IBO;
static int s_vertexCount = 0, s_indexCount = 0;

struct UISettings
{
    bool check1;
    float slider1;
} uiSettings;

struct PushConstBlock
{
	glm::vec2 scale;
	glm::vec2 translate;
} pushConstBlock;

namespace UIOverlay
{

    bool Init()
    {
        ImGui::CreateContext();
        ImGuiIO& io                = ImGui::GetIO();
		io.DisplaySize             = ImVec2( (float) GetMainWindow()->Width(), (float) GetMainWindow()->Height() );
		io.DisplayFramebufferScale = ImVec2( 1.0f, 1.0f );

        unsigned char* fontData;
        int texWidth, texHeight;
        io.Fonts->GetTexDataAsRGBA32( &fontData, &texWidth, &texHeight );

        // create texture to copy imgui font into
        ImageDescriptor imageDesc;
        imageDesc.format  = PixelFormat::R8_G8_B8_A8_UNORM;
        imageDesc.width   = texWidth;
        imageDesc.height  = texHeight;
        imageDesc.usage   = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageDesc.sampler = "linear_clamped_linear";
        s_fontTexture = g_renderState.device.NewTextureFromBuffer( imageDesc, fontData, false, "UI Font Texture" );
        if ( !s_fontTexture )
        {
            LOG_ERR( "Could not create ui font texture" );
            return false;
        }

        VertexBindingDescriptor bindingDescs[] =
        {
            VertexBindingDescriptor( 0, sizeof( ImDrawVert ) ),
        };

        VertexAttributeDescriptor attribDescs[] =
        {
            VertexAttributeDescriptor( 0, 0, BufferDataType::FLOAT2, offsetof( ImDrawVert, pos ) ),
            VertexAttributeDescriptor( 1, 0, BufferDataType::FLOAT2, offsetof( ImDrawVert, uv ) ),
            VertexAttributeDescriptor( 2, 0, BufferDataType::UCHAR4_NORM, offsetof( ImDrawVert, col ) ),
        };

        auto vertShader = ResourceManager::Get< Shader >( "uiVert" );
        auto fragShader = ResourceManager::Get< Shader >( "uiFrag" );

        std::vector< DescriptorSetLayoutData > descriptorSetData = vertShader->reflectInfo.descriptorSetLayouts;
        descriptorSetData.insert( descriptorSetData.end(), fragShader->reflectInfo.descriptorSetLayouts.begin(), fragShader->reflectInfo.descriptorSetLayouts.end() );
        auto combined = CombineDescriptorSetLayouts( descriptorSetData );
        s_descriptorSetLayouts = g_renderState.device.NewDescriptorSetLayouts( combined );

	    PipelineDescriptor pipelineDesc;
        pipelineDesc.renderPass	                 = &g_renderState.renderPass; // todo: check
        pipelineDesc.descriptorSetLayouts        = s_descriptorSetLayouts;
        pipelineDesc.vertexDescriptor            = VertexInputDescriptor::Create( 1, bindingDescs, 3, attribDescs );
        pipelineDesc.rasterizerInfo.cullFace     = CullFace::NONE;
        pipelineDesc.depthInfo.depthTestEnabled  = false;
        pipelineDesc.depthInfo.depthWriteEnabled = false;
        pipelineDesc.depthInfo.compareFunc       = CompareFunction::ALWAYS;
        pipelineDesc.viewport                    = FullScreenViewport();
        pipelineDesc.scissor                     = FullScreenScissor();
        pipelineDesc.shaders[0]	                 = vertShader.get();
        pipelineDesc.shaders[1]			         = fragShader.get();
        pipelineDesc.dynamicStates               = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

        pipelineDesc.colorAttachmentInfos[0].blendingEnabled = true;
        pipelineDesc.colorAttachmentInfos[0].dstAlphaBlendFactor = BlendFactor::ZERO;
        pipelineDesc.colorAttachmentInfos[0].srcAlphaBlendFactor = BlendFactor::ONE_MINUS_SRC_ALPHA;

	    s_pipeline = g_renderState.device.NewPipeline( pipelineDesc , "UIOverlay" );
	    if ( !s_pipeline )
	    {
		    LOG_ERR( "Could not create post UI pipeline" );
		    return false;
	    }

        VkDescriptorPoolSize poolSize = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 };
        s_descriptorPool = g_renderState.device.NewDescriptorPool( 1, &poolSize, 1, "UIOverlay" );

        s_descriptorSet = s_descriptorPool.NewDescriptorSet( s_descriptorSetLayouts[0], "UI Font texture descriptor layout" );

        VkDescriptorImageInfo imageInfo = DescriptorImageInfo( s_fontTexture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
        std::vector< VkWriteDescriptorSet > writeDescriptorSets =
        {
            WriteDescriptorSet( s_descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &imageInfo ),
        };
        g_renderState.device.UpdateDescriptorSets( static_cast< uint32_t >( writeDescriptorSets.size() ), writeDescriptorSets.data() );

        return true;
    }

    void Shutdown()
    {
        ImGui::DestroyContext();
        s_VBO.Free();
        s_IBO.Free();
        s_fontTexture.Free();
        s_pipeline.Free();
        FreeDescriptorSetLayouts( s_descriptorSetLayouts );
        s_descriptorPool.Free();
    }

    void UpdateBuffers()
    {
        ImDrawData* imDrawData = ImGui::GetDrawData();

		// Note: Alignment is done inside buffer creation
		size_t vertexBufferSize = imDrawData->TotalVtxCount * sizeof( ImDrawVert );
		size_t indexBufferSize  = imDrawData->TotalIdxCount * sizeof( ImDrawIdx );

		if ( vertexBufferSize == 0 || indexBufferSize == 0 )
        {
			return;
		}

		// Update buffers only if vertex or index count has been changed compared to current buffer size

		// Vertex buffer
		if ( !s_VBO || s_vertexCount != imDrawData->TotalVtxCount )
        {
            if ( s_VBO )
            {
                s_VBO.UnMap();
                s_VBO.Free();
            }
            s_VBO = g_renderState.device.NewBuffer( vertexBufferSize, BUFFER_TYPE_VERTEX, MEMORY_TYPE_HOST_VISIBLE, "UI Vertex Buffer" );
            PG_ASSERT( s_VBO );
			s_vertexCount = imDrawData->TotalVtxCount;
            s_VBO.Map();
		}

		// Index buffer
		VkDeviceSize indexSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);
		if ( !s_IBO || s_indexCount < imDrawData->TotalIdxCount )
        {
            if ( s_IBO )
            {
                s_IBO.UnMap();
                s_IBO.Free();
            }
            s_IBO = g_renderState.device.NewBuffer( indexBufferSize, BUFFER_TYPE_INDEX, MEMORY_TYPE_HOST_VISIBLE, "UI Index Buffer" );
            PG_ASSERT( s_IBO );
			s_indexCount = imDrawData->TotalIdxCount;
            s_IBO.Map();
		}

		// Upload data
		ImDrawVert* vtxDst = reinterpret_cast< ImDrawVert* >( s_VBO.MappedPtr() );
		ImDrawIdx* idxDst  = reinterpret_cast< ImDrawIdx* >( s_IBO.MappedPtr() );

		for ( int n = 0; n < imDrawData->CmdListsCount; n++ )
        {
			const ImDrawList* cmd_list = imDrawData->CmdLists[n];
			memcpy( vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof( ImDrawVert ) );
			memcpy( idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof( ImDrawIdx ) );
			vtxDst += cmd_list->VtxBuffer.Size;
			idxDst += cmd_list->IdxBuffer.Size;
		}

		// Flush to make writes visible to GPU
        s_VBO.Flush();
        s_IBO.Flush();
    }
    
    void Draw( CommandBuffer& cmdBuf )
    {
        // update imgui
        ImGuiIO& io     = ImGui::GetIO();
		io.DeltaTime    = Time::DeltaTime();
        auto mousePos   = Input::GetMousePosition();
		io.MousePos     = ImVec2( mousePos.x, mousePos.y );
		io.MouseDown[0] = Input::GetMouseButtonHeld( MouseButton::LEFT );
		io.MouseDown[1] = Input::GetMouseButtonHeld( MouseButton::RIGHT );

        // draw
        ImGui::NewFrame();

        ImGui::SetNextWindowSize(ImVec2(200, 200), ImGuiCond_FirstUseEver );
		ImGui::Begin("Example settings");
		ImGui::Checkbox("Render models", &uiSettings.check1 );
		ImGui::SliderFloat("Light speed", &uiSettings.slider1, 0.1f, 1.0f);
		ImGui::End();
        
		ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver );
		ImGui::ShowDemoWindow();
        // ImGui::ShowDemoWindow();

        // Render to generate draw buffers
		ImGui::Render();

        UpdateBuffers();

        ImDrawData* imDrawData = ImGui::GetDrawData();
		if ( !imDrawData || imDrawData->CmdListsCount == 0 )
        {
			return;
		}

        cmdBuf.BindRenderPipeline( s_pipeline );
        cmdBuf.BindDescriptorSets( 1, &s_descriptorSet, s_pipeline );

        Viewport viewport( ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y );
        cmdBuf.SetViewport( viewport );

		pushConstBlock.scale     = glm::vec2( 2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y );
		pushConstBlock.translate = glm::vec2( -1.0f );
        cmdBuf.PushConstants( s_pipeline, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof( PushConstBlock ), &pushConstBlock );

        cmdBuf.BindVertexBuffer( s_VBO );
        cmdBuf.BindIndexBuffer( s_IBO, IndexType::UNSIGNED_SHORT );

        int vertexOffset = 0;
		int indexOffset  = 0;
		for ( int i = 0; i < imDrawData->CmdListsCount; i++ )
		{
			const ImDrawList* cmd_list = imDrawData->CmdLists[i];
			for ( int j = 0; j < cmd_list->CmdBuffer.Size; j++ )
			{
				const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];
                Scissor scissor;
                scissor.x      = std::max( static_cast< int >( pcmd->ClipRect.x ), 0 );
                scissor.y      = std::max( static_cast< int >( pcmd->ClipRect.y ), 0 );
                scissor.width  = static_cast< int >( pcmd->ClipRect.z - pcmd->ClipRect.x );
                scissor.height = static_cast< int >( pcmd->ClipRect.w - pcmd->ClipRect.y );
                cmdBuf.SetScissor( scissor );
                cmdBuf.DrawIndexed( indexOffset, pcmd->ElemCount, vertexOffset );
				indexOffset += pcmd->ElemCount;
			}
			vertexOffset += cmd_list->VtxBuffer.Size;
		}
    }

} // namespace UIOverlay
} // namespace Gfx
} // namespace Progression