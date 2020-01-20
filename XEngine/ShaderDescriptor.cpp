#include "ShaderDescriptor.h"

void ShaderDescriptor::AddUniformBuffer(TBuffer buffer, int bindingPoint, int offset)
{
	m_uniformBuffers.push_back(std::make_tuple(buffer, bindingPoint, offset));
}

void ShaderDescriptor::AddShaderStorageBuffer(TBuffer buffer, int bindingPoint, int offset)
{
	m_ssBuffers.push_back(std::make_tuple(buffer, bindingPoint, offset));
}

void ShaderDescriptor::AddAtomicCounterBuffer(TBuffer buffer, int bindingPoint, int offset)
{
	m_acBuffers.push_back(std::make_tuple(buffer, bindingPoint, offset));
}

void ShaderDescriptor::BindDescriptor()
{
	TBuffer buffer;
	int binding;
	int offset;

	for (std::tuple<TBuffer, int, int> uniformBuffer : m_uniformBuffers)
	{
		auto [buffer, binding, offset] = uniformBuffer;
		glBindBufferBase(GL_UNIFORM_BUFFER, binding, buffer.GetID());
	}
	for (std::tuple<TBuffer, int, int> ssBuffer : m_ssBuffers)
	{
		auto [buffer, binding, offset] = ssBuffer;
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, buffer.GetID());
	}
	for (std::tuple<TBuffer, int, int> acBuffer : m_acBuffers)
	{
		auto [buffer, binding, offset] = acBuffer;
		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, binding, buffer.GetID());
	}
}
