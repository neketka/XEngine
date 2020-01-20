#pragma once
#include <functional>

#include "Pipeline.h"
#include "ShaderConstants.h"
#include "ShaderDescriptor.h"

enum class PrimitiveType
{
	Triangles = GL_TRIANGLES
};

enum class MemoryBarrierType : unsigned
{
	VertexAttribArray = GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT,
	IndexBuffer = GL_ELEMENT_ARRAY_BARRIER_BIT,
	Uniform = GL_UNIFORM_BARRIER_BIT,
	TextureFetch = GL_TEXTURE_FETCH_BARRIER_BIT,
	ShaderImageAccess = GL_SHADER_IMAGE_ACCESS_BARRIER_BIT,
	CommandBarrier = GL_COMMAND_BARRIER_BIT,
	PixelBuffer = GL_PIXEL_BUFFER_BARRIER_BIT,
	TextureUpdate = GL_TEXTURE_UPDATE_BARRIER_BIT,
	BufferUpdate = GL_BUFFER_UPDATE_BARRIER_BIT,
	QueryBuffer = GL_QUERY_BUFFER_BARRIER_BIT,
	ClientMappedBuffer = GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT,
	Framebuffer = GL_FRAMEBUFFER_BARRIER_BIT,
	TransformFeedback = GL_TRANSFORM_FEEDBACK_BARRIER_BIT,
	AtomicCounter = GL_ATOMIC_COUNTER_BARRIER_BIT,
	ShaderStorageBuffer = GL_SHADER_STORAGE_BARRIER_BIT,
	All = GL_ALL_BARRIER_BITS
};

class SyncObject
{
private:
	GLsync m_sync;
};

class CommandBuffer
{
public:
	CommandBuffer() {
		m_commands.reserve(40);
		m_descriptors.reserve(20);
		m_pipelines.reserve(20);
		m_constants.reserve(20);
	}
	void ClearColorFramebufferCommand(int attachment, glm::vec4 color);
	void ClearDepthFramebufferCommand(float value);
	void ClearAllBuffers();

	void BeginTransformFeedback(PrimitiveType type);
	void EndTransformFeedback();

	void BindPixelUnpackBufferCommand(TBuffer buffer);
	void BindPixelPackBufferCommand(TBuffer buffer);

	void BindIndirectCommandBufferCommand(TBuffer buffer);

	void BindComputeShaderCommand(Shader shader);
	void BindPipelineCommand(Pipeline pipeline);
	void BindConstantsCommand(ShaderConstants constants);
	void BindDescriptorCommand(ShaderDescriptor descriptor);

	void DrawIndexedCommand(PrimitiveType type, int count, int instances, int baseIndex, int baseVertex, int baseInstance);
	void DrawCommand(PrimitiveType type, int first, int count, int instances, int baseInstance);
	void DrawIndexedIndirectCommand(PrimitiveType type, int offset, int cmdCount, int stride);
	void DrawIndirectCommand(PrimitiveType type, int offset, int cmdCount, int stride);
	void DispatchCommand(int x, int y, int z);

	void MemoryBarrier(MemoryBarrierType type);
	void TextureBarrier();
	
	//void ExecuteCommandBufferCommand(CommandBuffer buffer);

	void ExecuteCommands();

	void ClearCommandBuffer();
private:
	std::vector<std::function<void()>> m_commands;

	std::vector<ShaderConstants> m_constants;
	std::vector<ShaderDescriptor> m_descriptors;
	std::vector<Pipeline> m_pipelines;
	std::vector<CommandBuffer> m_subBuffers;
};

