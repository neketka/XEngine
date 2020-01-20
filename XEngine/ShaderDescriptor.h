#pragma once
#include "Buffer.h"

class ShaderDescriptor
{
public:
	void AddUniformBuffer(TBuffer buffer, int bindingPoint, int offset = 0);
	void AddShaderStorageBuffer(TBuffer buffer, int bindingPoint, int offset = 0);
	void AddAtomicCounterBuffer(TBuffer buffer, int bindingPoint, int offset = 0);
	void BindDescriptor();
private:
	std::vector<std::tuple<TBuffer, int, int>> m_uniformBuffers;
	std::vector<std::tuple<TBuffer, int, int>> m_ssBuffers;
	std::vector<std::tuple<TBuffer, int, int>> m_acBuffers;
};

