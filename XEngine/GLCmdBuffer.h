#pragma once

#include "GraphicsDefs.h"
#include "GLPipeline.h"
#include "GLImage.h"

class GLCmdBuffer : public GraphicsCommandBuffer 
{
public:
	GLCmdBuffer(GraphicsContext *context, bool pooled=false);
	virtual ~GLCmdBuffer() override;
	virtual void BindRenderPipeline(GraphicsRenderPipeline *pipeline) override;
	virtual void BindComputePipeline(GraphicsComputePipeline *pipeline) override;
	virtual void BindRenderPass(GraphicsRenderTarget *target, GraphicsRenderPass *renderPass, std::vector<glm::vec4>&& attachmentClearValues, char stencil) override;
	virtual void NextSubpass() override;
	virtual void EndRenderPass() override;
	virtual void BeginQuery(GraphicsQuery *query) override;
	virtual void EndQuery(GraphicsQuery *query) override;
	virtual void ResetQuery(GraphicsQuery *query) override;
	virtual void WriteTimestamp(GraphicsQuery *query) override;
	virtual void WriteQueryToBuffer(GraphicsQuery *query, int32_t bufferOffset) override;
	virtual void BindVertexBuffers(int32_t firstBinding, std::vector<GraphicsMemoryBuffer *> buffers, std::vector<int32_t> offsets) override;
	virtual void BindIndexBuffer(GraphicsMemoryBuffer *buffer, bool dataType16bit) override;
	virtual void PushShaderConstants(GraphicsShaderDataSet *set, int32_t constantIndex, int32_t constantOffset, int32_t constantCount, void *data) override;
	virtual void BindRenderShaderResourceInstance(GraphicsShaderDataSet *set, GraphicsShaderResourceInstance *instance, int32_t viewIndex, int32_t offset) override;
	virtual void BindComputeShaderResourceInstance(GraphicsShaderDataSet *set, GraphicsShaderResourceInstance *instance, int32_t viewIndex, int32_t offset) override;
	virtual void DrawIndexed(int32_t vertexCount, int32_t instances, int32_t firstIndex, int32_t vertexOffset, int32_t firstInstance) override;
	virtual void Draw(int32_t vertexCount, int32_t instanceCount, int32_t firstVertex, int32_t firstInstance) override;
	virtual void DrawIndirect(GraphicsMemoryBuffer *buffer, int32_t offset, int32_t drawCount, int32_t stride) override;
	virtual void DrawIndirectIndexed(GraphicsMemoryBuffer *buffer, int32_t offset, int32_t drawCount, int32_t stride) override;
	virtual void DrawIndirectCount(GraphicsMemoryBuffer *buffer, int32_t offset, GraphicsMemoryBuffer * drawCountBuffer, int32_t drawCountBufferOffset, int32_t maxDrawCount, int32_t stride) override;
	virtual void DrawIndirectIndexedCount(GraphicsMemoryBuffer *buffer, int32_t offset, GraphicsMemoryBuffer * drawCountBuffer, int32_t drawCountBufferOffset, int32_t maxDrawCount, int32_t stride) override;
	virtual void DispatchCompute(int32_t x, int32_t y, int32_t z) override;
	virtual void DispatchIndirect(GraphicsMemoryBuffer *buffer, int32_t offset) override;
	virtual void UpdatePipelineDynamicViewport(glm::vec4 dimensions) override;
	virtual void UpdatePipelineDynamicScissorBox(glm::vec4 dimensions) override;
	virtual void UpdatePipelineDynamicLineWidth(float lineWidth) override;
	virtual void UpdatePipelineDynamicStencilFrontFaceCompareMask(uint32_t mask) override;
	virtual void UpdatePipelineDynamicStencilFrontFaceWriteMask(uint32_t mask) override;
	virtual void UpdatePipelineDynamicStencilFrontFaceReference(uint32_t mask) override;
	virtual void UpdatePipelineDynamicStencilBackFaceCompareMask(uint32_t mask) override;
	virtual void UpdatePipelineDynamicStencilBackFaceWriteMask(uint32_t mask) override;
	virtual void UpdatePipelineDynamicStencilBackFaceReference(uint32_t mask) override;
	virtual void UpdatePipelineDynamicBlendConstant(glm::vec4 constants) override;
	virtual void WaitOnFence(GraphicsSyncObject *sync) override;
	virtual void SignalFence(GraphicsSyncObject *sync) override;
	virtual void ResetFence(GraphicsSyncObject *sync) override;
	virtual void SynchronizeMemory(MemoryBarrierBit from, MemoryBarrierBit to, bool byRegion) override;
	virtual void ClearColor(GraphicsImageObject *image, glm::vec4 color, int32_t level, int32_t layer) override;
	virtual void ClearDepthStencil(GraphicsImageObject *image, float depth, char stencil) override;
	virtual void ClearAttachmentsColor(int32_t index, glm::vec4 color) override;
	virtual void ClearAttachmentsDepthStencil(int32_t index, float depth, char stencil) override;
	virtual void UpdateBufferData(GraphicsMemoryBuffer *buffer, int32_t offset, int32_t size, void *data) override;
	virtual void CopyBufferToImageWithConversion(GraphicsMemoryBuffer *srcBuffer, VectorDataFormat srcFormat, GraphicsImageObject *destImage, std::vector<GraphicsBufferImageCopyRegion> regions) override;
	virtual void CopyImageToBuffer(GraphicsImageObject *srcImage, GraphicsMemoryBuffer *destBuffer, std::vector<GraphicsBufferImageCopyRegion> regions) override;
	virtual void CopyImageToImage(GraphicsImageObject *srcImage, GraphicsImageObject *destImage, glm::ivec3 srcOffset, glm::ivec3 destOffset, glm::ivec3 size, int32_t srcLevel, int32_t destLevel, int32_t layers, bool color, bool depth, bool stencil) override;
	virtual void CopyBufferToBuffer(GraphicsMemoryBuffer *src, GraphicsMemoryBuffer *dest, uint64_t srcOffset, uint64_t destOffset, int32_t size) override;
	virtual void BeginRecording() override;
	virtual void StopRecording() override;
	virtual void ExecuteSubCommandBuffer(GraphicsCommandBuffer *commandBuffer) override;
	bool IsPooled() { return m_pooled; }

	void Execute();
private:
	GraphicsContext *m_context;
	std::vector<void *> m_buffersHeld;
	std::vector<std::function<void()>> m_commands;
	GLPipeline *m_lastRenderPipe;
	GLComputePipeline *m_lastComputePipe;
	GLRenderTarget *m_lastRenderTarget;
	GLRenderPass *m_lastPass;
	int32_t m_subpassIndexCounter = 0;
	bool m_ibo16Bit;
	bool m_lastPipeCompute = false;
	uint32_t m_frontCompMask;
	uint32_t m_frontRefMask;
	uint32_t m_backCompMask;
	uint32_t m_backRefMask;
	bool m_pooled;

	int32_t m_minTextureUnit = 0;
	int32_t m_minImageUnit = 0;
};