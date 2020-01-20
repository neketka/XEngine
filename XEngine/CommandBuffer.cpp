#include "CommandBuffer.h"

void CommandBuffer::ClearColorFramebufferCommand(int attachment, glm::vec4 color)
{
	m_commands.push_back([color, attachment]() {
		float colorv[] = { color.x, color.y, color.z, color.w };
		glClearBufferfv(GL_COLOR, attachment, colorv);
	});
}

void CommandBuffer::ClearDepthFramebufferCommand(float value)
{
	m_commands.push_back([value]() {
		glClearBufferfv(GL_DEPTH, 0, &value);
	});
}

void CommandBuffer::ClearAllBuffers()
{
	m_commands.push_back([]() {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	});
}

void CommandBuffer::BeginTransformFeedback(PrimitiveType type)
{
	m_commands.push_back([type]() {
		glBeginTransformFeedback(static_cast<GLenum>(type));
	});
}

void CommandBuffer::EndTransformFeedback()
{
	m_commands.push_back([]() {
		glEndTransformFeedback();
	});
}

void CommandBuffer::BindPixelUnpackBufferCommand(TBuffer buffer)
{
	auto id = buffer.GetID();
	m_commands.push_back([id]() {
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, id);
	});
}

void CommandBuffer::BindPixelPackBufferCommand(TBuffer buffer)
{
	auto id = buffer.GetID();
	m_commands.push_back([id]() {
		glBindBuffer(GL_PIXEL_PACK_BUFFER, id);
	});
}

void CommandBuffer::BindIndirectCommandBufferCommand(TBuffer buffer)
{
	auto id = buffer.GetID();
	m_commands.push_back([id]() {
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, id);
	});
}

void CommandBuffer::BindComputeShaderCommand(Shader shader)
{
	auto id = shader.GetID();
	m_commands.push_back([id]() {
		glUseProgram(id);
	});
}

void CommandBuffer::BindPipelineCommand(Pipeline pipeline)
{
	m_pipelines.push_back(pipeline);
	int index = m_pipelines.size() - 1;
	std::vector<Pipeline> *pipelines = &m_pipelines;

	m_commands.push_back([index, pipelines]() {
		(*pipelines)[index].BindPipeline();
	});
}

void CommandBuffer::BindConstantsCommand(ShaderConstants constants)
{
	m_constants.push_back(constants);
	int index = m_constants.size() - 1;
	std::vector<ShaderConstants> *constantsVec = &m_constants;

	m_commands.push_back([index, constantsVec]() {
		(*constantsVec)[index].BindConstants();
	});
}

void CommandBuffer::BindDescriptorCommand(ShaderDescriptor descriptor)
{
	m_descriptors.push_back(descriptor);
	int index = m_descriptors.size() - 1;
	std::vector<ShaderDescriptor> *descriptors = &m_descriptors;

	m_commands.push_back([index, descriptors]() {
		(*descriptors)[index].BindDescriptor();
	});
}

void CommandBuffer::DrawIndexedCommand(PrimitiveType type, int count, int instances, int baseIndex, int baseVertex, int baseInstance)
{
	m_commands.push_back([type, count, instances, baseIndex, baseVertex, baseInstance]() {
		glDrawElementsInstancedBaseVertexBaseInstance(static_cast<GLenum>(type), count, GL_UNSIGNED_INT,
			reinterpret_cast<GLvoid *>(baseIndex), instances, baseVertex, baseInstance);
	});
}

void CommandBuffer::DrawCommand(PrimitiveType type, int first, int count, int instances, int baseInstance)
{
	m_commands.push_back([type, count, instances, first, baseInstance]() {
		glDrawArraysInstancedBaseInstance(static_cast<GLenum>(type), first, count,
			instances, baseInstance);
	});
}

void CommandBuffer::DrawIndexedIndirectCommand(PrimitiveType type, int offset, int cmdCount, int stride)
{
	m_commands.push_back([type, cmdCount, stride, offset]() {
		glMultiDrawElementsIndirect(static_cast<GLenum>(type), GL_UNSIGNED_INT, reinterpret_cast<const void *>(offset), cmdCount, stride);
	});
}

void CommandBuffer::DrawIndirectCommand(PrimitiveType type, int offset, int cmdCount, int stride)
{
	m_commands.push_back([type, cmdCount, stride, offset]() {
		glMultiDrawArraysIndirect(static_cast<GLenum>(type), reinterpret_cast<const void *>(offset), cmdCount, stride);
	});
}

void CommandBuffer::DispatchCommand(int x, int y, int z)
{
	m_commands.push_back([x, y, z]() {
		glDispatchCompute(x, y, z);
	});
}
void CommandBuffer::MemoryBarrier(MemoryBarrierType type)
{
	m_commands.push_back([type]() {
		glMemoryBarrier(static_cast<GLbitfield>(type));
	});
}
void CommandBuffer::TextureBarrier()
{
	m_commands.push_back([]() {
		glTextureBarrier();
	});
}
/*
void CommandBuffer::ExecuteCommandBufferCommand(CommandBuffer buffer)
{
	m_subBuffers.push_back(buffer);
	CommandBuffer *b = &m_subBuffers[m_subBuffers.size() - 1];
	m_commands.push_back([b]() {
		b->ExecuteCommands();
	});
}
*/
void CommandBuffer::ExecuteCommands()
{
	for (std::function<void()> func : m_commands)
		func();
}

void CommandBuffer::ClearCommandBuffer()
{
	m_commands.clear();
	m_constants.clear();
	m_descriptors.clear();
	m_pipelines.clear();
	m_subBuffers.clear();
}
