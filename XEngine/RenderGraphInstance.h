#pragma once

#include "RenderGraphStructs.h"

class RenderGraphInstance;
class RenderTaskInternal;
class RenderTask
{
public:
	RenderTask(RenderTaskInternal *renderTask, RenderGraphInstance *graphInstance);
	~RenderTask();

	uint32_t GetFrameImageCount();
	uint32_t GetCurrentFrameImageIndex();
	
	RenderResource CreateGraphicsExecutionState(GraphicsExecutionStateData data);
	RenderResource CreateComputeExecutionState(ComputeExecutionStateData data);
	RenderResource CreateBuffer(uint64_t size, bool frequentUpdates, BufferUsageBit usage);
	RenderResource CreateImage(ImageType type, glm::ivec3 size, int32_t levels, int32_t layers, RenderFormat format, ImageUsageBit usage);
	RenderResource CreateSampler(SamplerState state);
	RenderResource CreateShaderViewCollectionLayout(ShaderViewCollectionData data);
	RenderResource CreateShaderViewCollection(RenderResource layout);
	RenderResource CreateQuerySet(QueryType type, int32_t count, PipelineStatisticBits statistics);
	RenderResource CreateVulkanShader(ShaderStageBit stage, std::vector<char> spirvCode, 
		std::vector<VulkanSpecializationConstantDataDescription> constantDescriptions, void *constantData, uint32_t constDataSize);

	void BeginTask();
	ComputeTask BeginComputeTask();
	TransferTask BeginTransferTask();
	void EndTask();

	void BeginQuery(RenderResource querySet, int32_t index);
	void EndQuery(RenderResource querySet, int32_t index);
	void ResetQueryRange(RenderResource querySet, int32_t index, int32_t count);
	void WriteTimestamp(RenderResource querySet, int32_t index);
	
	void BindShaderViewCollection(RenderResource collection, uint32_t dynamicOffset, uint32_t index);
	void UpdateShaderViewCollectionImage(RenderResource shaderViewCollection, uint32_t bindingIndex, uint32_t firstArrayIndex, 
		std::vector<ImageShaderView> images);
	void UpdateShaderViewCollectionBuffer(RenderResource shaderViewCollection, uint32_t bindingIndex, uint32_t firstArrayIndex,
		std::vector<BufferShaderView> buffers);

	void PushShaderConstants(ShaderStageBit stages, uint32_t offset, uint32_t size, void *data);
	void PushMarker(glm::vec4 color, std::string name);
	void PopMarker();

	void DeclareShaderViewArrayUsage(uint32_t svCollectionIndex, uint32_t binding, uint32_t first, uint32_t count);

	void BindComputeExecutionState(RenderResource state);
	void Dispatch(int32_t x, int32_t y, int32_t z);
	void DispatchIndirect(RenderResource indirectBuffer, uint64_t bufferOffset);

	void BindGraphicsExecutionState(RenderResource state);
	void UpdateViewport(glm::ivec2 offset, glm::ivec2 size);
	void UpdateScissorBox(glm::ivec2 offset, glm::ivec2 size);
	void BindVertexBuffer(RenderResource vertexBuffer, uint64_t bufferOffset, uint32_t binding);
	void BindIndexBuffer(RenderResource indexBuffer, uint64_t bufferOffset, bool shortIndexType);
	void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t baseInstance);
	void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t baseInstance);
	void DrawIndirect(RenderResource indirectBuffer, uint64_t bufferOffset, uint32_t drawCount, uint32_t stride);
	void DrawIndexedIndirect(RenderResource indirectBuffer, uint64_t bufferOffset, uint32_t drawCount, uint32_t stride);
	void DrawIndirectCount(RenderResource indirectBuffer, uint64_t indirectBufferOffset, RenderResource countBuffer, uint64_t countBufferOffset,
		uint32_t maxDrawCount, uint32_t indirectStride);
	void DrawIndexedIndirectCount(RenderResource indirectBuffer, uint64_t indirectBufferOffset, RenderResource countBuffer, uint64_t countBufferOffset,
		uint32_t maxDrawCount, uint32_t indirectStride);
	void BeginRenderPass(RenderPassBeginInfo info);
	void EndRenderPass();

	void WriteQueryToBuffer(RenderResource querySet, RenderResource buffer, uint64_t bufferOffset);
	void CopyImage(RenderResource src, RenderResource dest, std::vector<ImageCopyOperation> copyRegions);
	void CopyBuffer(RenderResource src, RenderResource dest, uint64_t srcOffset, uint64_t destOffset, uint64_t size);

	void UpdateBuffer(RenderResource buffer, uint64_t offset, uint32_t size, void *data);
	void UpdateImage(RenderResource image, ImageSubresourceLevel subResource, void *data);
	void ReadBuffer(RenderResource buffer, uint64_t offset, uint32_t size, void *dest);
	void ReadImage(RenderResource image, ImageSubresourceLevel subResource, RenderFormat dataFormat, void *dest);
private:
	RenderTaskInternal *m_renderTask;
	RenderGraphInstance *m_graphInstance;
};
