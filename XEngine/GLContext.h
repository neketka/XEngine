#pragma once

#include "GraphicsDefs.h"
#include "GLCmdBuffer.h"
#include "GLInitable.h"
#include "GLBuffer.h"

#include <SDL_video.h>

#include <concurrent_queue.h>
#include <thread>

class GLSpecific : public GraphicsSpecificStructure
{
public:
	virtual std::string GetApiString() override;
};

class GLContext : public GraphicsContext
{
public:
	GLContext(SDL_Window *window);
	~GLContext();
	virtual std::vector<GraphicsCommandBuffer *> CreateGraphicsCommandBuffers(int count, bool graphics, bool compute, bool transfer) override;
	virtual GraphicsCommandBuffer *GetGraphicsBufferFromPool() override;
	virtual GraphicsCommandBuffer *GetTransferBufferFromPool() override;
	virtual GraphicsCommandBuffer *GetComputeBufferFromPool() override;
	virtual GraphicsRenderPipeline *CreateGraphicsPipeline(GraphicsRenderPipelineState& state) override;
	virtual GraphicsComputePipeline *CreateComputePipeline(GraphicsComputePipelineState& state) override;
	virtual GraphicsMemoryBuffer *CreateBuffer(unsigned long long byteSize, BufferUsageBit usage, GraphicsMemoryTypeBit mem) override;
	virtual GraphicsImageObject *CreateImage(ImageType type, VectorDataFormat format, glm::ivec3 size, int miplevels, ImageUsageBit usage) override;
	virtual GraphicsRenderTarget *CreateRenderTarget(std::vector<GraphicsImageView *>&& attachments, GraphicsImageView *depthStencil, GraphicsRenderPass *renderPass, int width, int height, int layers) override;
	virtual GraphicsShaderDataSet *CreateShaderDataSet(std::vector<GraphicsShaderResourceViewData>&& resourceViews, std::vector<GraphicsShaderConstantData>&& constantData) override;
	virtual GraphicsShaderResourceInstance *CreateShaderResourceInstance(GraphicsShaderResourceViewData& data) override;
	virtual GraphicsSampler *CreateSampler(GraphicsSamplerState& state) override;
	virtual GraphicsShader *CreateShader(GraphicsSpecificShaderCode *code) override;
	virtual GraphicsSyncObject *CreateSync(bool gpuQueueSync) override;
	virtual GraphicsRenderPass *CreateRenderPass(GraphicsRenderPassState& state) override;
	virtual std::vector<GraphicsQuery *> CreateQueries(int count, GraphicsQueryType type) override;
	virtual GraphicsImageView *GetScreenImageView() override;
	virtual glm::ivec2 GetScreenSize() override;
	virtual GraphicsSpecificStructure& GetSpecificStructure() override;
	virtual void UpdateShaderImageSamplerResourceInstance(GraphicsShaderResourceInstance *instance, std::vector<GraphicsImageView *>&& imageViews, std::vector<GraphicsSampler *>&& samplers) override;
	virtual void UpdateShaderImageInputAttachmentInstance(GraphicsShaderResourceInstance *instance, std::vector<GraphicsImageView *>&& imageViews) override;
	virtual void UpdateShaderImageLoadStoreResourceInstance(GraphicsShaderResourceInstance *instance, std::vector<GraphicsImageView *>&& imageViews) override;
	virtual void UpdateShaderBufferResourceInstance(GraphicsShaderResourceInstance *instance, GraphicsMemoryBuffer *buffer, int offset, int range) override;
	virtual void SubmitCommands(GraphicsCommandBuffer *commands, GraphicsQueueType queue) override;
	virtual void Present() override;
	virtual void SyncWithCommandSubmissionThread();

	void ResizeScreen(glm::ivec2 size);
	void WaitUntilFramesFinishIfEqualTo(int bufferedFrames);

	void DeleteInitable(std::function<void()> initable);
	void EnqueueInitable(GLInitable *initable);
	void WaitForSync(GraphicsSyncObject *sync);
	void MapRequest(GLBuffer *buffer);
private:
	void RunContextThread();
	GraphicsCommandBuffer *GetPoolCmdBuf();
	std::atomic<glm::ivec2> m_atomicScreenSize;

	std::atomic_bool m_initScreen;
	std::atomic_bool m_syncWithRenderThread;

	GraphicsImageObject *m_colorImage;
	GraphicsImageView *m_colorView;
	GraphicsRenderTarget *m_renderTarget;

	std::atomic_bool m_running = false;
	GLSpecific *m_specific;
	SDL_Window *m_window; 
	SDL_GLContext m_context;

	concurrency::concurrent_queue<GLCmdBuffer *> m_cmdPool;

	concurrency::concurrent_queue<GLInitable *> m_queuedInitializers;
	concurrency::concurrent_queue<GLCmdBuffer *> m_queuedBuffers;
	concurrency::concurrent_queue<std::function<void()>> m_queuedDeleters;
	concurrency::concurrent_queue<GraphicsSyncObject *> m_queuedSyncs;
	concurrency::concurrent_queue<GLBuffer *> m_queuedMaps;

	std::thread *m_glSubmissionThread;

	std::atomic_int m_framesInProgress;
};