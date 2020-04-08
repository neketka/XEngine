#pragma once
#include "GraphicsEnums.h"
#include <vector>

class GraphicsSync
{
public:
};

class GraphicsShaderSmallConstantData
{
public:
};

class GraphicsShaderResourceView
{
public:
};

class GraphicsRenderTarget
{
public:
};

class GraphicsImageObject
{
public:
};

class GraphicsImageView
{
public:
};

class GraphicsMemoryBuffer
{
public:

};

class GraphicsVertexBufferView
{
public:
};

class GraphicsIndexBufferView
{
public:
};

class GraphicsShaderState
{
public:
};

class GraphicsQuery
{
public:
};

class GraphicsRenderPipelineState
{
public:
};

class GraphicsCommandBuffer
{
public:
};

class GraphicsContext
{
public:
	virtual ~GraphicsContext() {}
	virtual std::vector<GraphicsCommandBuffer *> CreateCommandBuffers(int count, bool subBuffer) = 0;
	virtual void SubmitCommands(GraphicsCommandBuffer *commands, GraphicsQueueType queue) = 0;
	virtual void Present() = 0;
};