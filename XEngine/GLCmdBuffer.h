#pragma once

#include "GraphicsDefs.h"
#include "GLPipeline.h"
#include "GLImage.h"

class GLCmdBuffer : public GraphicsCommandBuffer 
{
public:
	GLCmdBuffer(GraphicsContext *context);
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
	virtual void WriteQueryToBuffer(GraphicsQuery *query, int bufferOffset) override;
	virtual void BindVertexBuffers(int firstBinding, std::vector<GraphicsMemoryBuffer *> buffers, std::vector<int> offsets) override;
	virtual void BindIndexBuffer(GraphicsMemoryBuffer *buffer, bool dataType16bit) override;
	virtual void PushShaderConstants(GraphicsShaderDataSet *set, int constantIndex, int constantOffset, int constantCount, void *data) override;
	virtual void BindRenderShaderResourceInstance(GraphicsShaderDataSet *set, GraphicsShaderResourceInstance *instance, int viewIndex, int offset) override;
	virtual void BindComputeShaderResourceInstance(GraphicsShaderDataSet *set, GraphicsShaderResourceInstance *instance, int viewIndex, int offset) override;
	virtual void DrawIndexed(int vertexCount, int instances, int firstIndex, int vertexOffset, int firstInstance) override;
	virtual void Draw(int vertexCount, int instanceCount, int firstVertex, int firstInstance) override;
	virtual void DrawIndirect(GraphicsMemoryBuffer *buffer, int offset, int drawCount, int stride) override;
	virtual void DrawIndirectIndexed(GraphicsMemoryBuffer *buffer, int offset, int drawCount, int stride) override;
	virtual void DrawIndirectCount(GraphicsMemoryBuffer *buffer, int offset, GraphicsMemoryBuffer * drawCountBuffer, int drawCountBufferOffset, int maxDrawCount, int stride) override;
	virtual void DrawIndirectIndexedCount(GraphicsMemoryBuffer *buffer, int offset, GraphicsMemoryBuffer * drawCountBuffer, int drawCountBufferOffset, int maxDrawCount, int stride) override;
	virtual void DispatchCompute(int x, int y, int z) override;
	virtual void DispatchIndirect(GraphicsMemoryBuffer *buffer, int offset) override;
	virtual void UpdatePipelineDynamicViewport(glm::vec4 dimensions) override;
	virtual void UpdatePipelineDynamicScissorBox(glm::vec4 dimensions) override;
	virtual void UpdatePipelineDynamicLineWidth(float lineWidth) override;
	virtual void UpdatePipelineDynamicStencilFrontFaceCompareMask(unsigned int mask) override;
	virtual void UpdatePipelineDynamicStencilFrontFaceWriteMask(unsigned int mask) override;
	virtual void UpdatePipelineDynamicStencilFrontFaceReference(unsigned int mask) override;
	virtual void UpdatePipelineDynamicStencilBackFaceCompareMask(unsigned int mask) override;
	virtual void UpdatePipelineDynamicStencilBackFaceWriteMask(unsigned int mask) override;
	virtual void UpdatePipelineDynamicStencilBackFaceReference(unsigned int mask) override;
	virtual void UpdatePipelineDynamicBlendConstant(glm::vec4 constants) override;
	virtual void WaitOnFence(GraphicsSyncObject *sync) override;
	virtual void SignalFence(GraphicsSyncObject *sync) override;
	virtual void ResetFence(GraphicsSyncObject *sync) override;
	virtual void SynchronizeMemory(MemoryBarrierBit from, MemoryBarrierBit to, bool byRegion) override;
	virtual void ClearColor(GraphicsImageObject *image, glm::vec4 color, int level, int layer) override;
	virtual void ClearDepthStencil(GraphicsImageObject *image, float depth, char stencil) override;
	virtual void ClearAttachmentsColor(int index, glm::vec4 color) override;
	virtual void ClearAttachmentsDepthStencil(int index, float depth, char stencil) override;
	virtual void UpdateBufferData(GraphicsMemoryBuffer *buffer, int offset, int size, void *data) override;
	virtual void CopyBufferToImageWithConversion(GraphicsMemoryBuffer *srcBuffer, VectorDataFormat srcFormat, GraphicsImageObject *destImage, std::vector<GraphicsBufferImageCopyRegion> regions) override;
	virtual void CopyImageToBuffer(GraphicsImageObject *srcImage, GraphicsMemoryBuffer *destBuffer, std::vector<GraphicsBufferImageCopyRegion> regions) override;
	virtual void CopyImageToImage(GraphicsImageObject *srcImage, GraphicsImageObject *destImage, glm::ivec3 srcOffset, glm::ivec3 destOffset, glm::ivec3 size, int srcLevel, int destLevel, int layers, bool color, bool depth, bool stencil) override;
	virtual void CopyBufferToBuffer(GraphicsMemoryBuffer *src, GraphicsMemoryBuffer *dest, unsigned long long srcOffset, unsigned long long destOffset, int size) override;
	virtual void BeginRecording() override;
	virtual void StopRecording() override;
	virtual void ExecuteSubCommandBuffer(GraphicsCommandBuffer *commandBuffer) override;

	void Execute();
private:
	GraphicsContext *m_context;
	std::vector<void *> m_buffersHeld;
	std::vector<std::function<void()>> m_commands;
	GLPipeline *m_lastRenderPipe;
	GLComputePipeline *m_lastComputePipe;
	GLRenderTarget *m_lastRenderTarget;
	GLRenderPass *m_lastPass;
	int m_subpassIndexCounter = 0;
	bool m_ibo16Bit;
	bool m_lastPipeCompute = false;
	unsigned int m_frontCompMask;
	unsigned int m_frontRefMask;
	unsigned int m_backCompMask;
	unsigned int m_backRefMask;

	int m_minTextureUnit = 0;
	int m_minImageUnit = 0;
};