#pragma once

#include "ECS.h"
#include "RenderGraphInstance.h"

#include <shared_mutex>
#include <vulkan/vulkan.hpp>

class RenderTaskInternal;
class RenderGraphInstance;

enum class ResourceBarrierType
{
	Image, Buffer, Memory, Execution
};

class ResourceState
{
public:
	vk::ImageLayout ImageLayout;
	vk::AccessFlagBits AccessFlags;
	vk::PipelineStageFlagBits PipeStageFlags;
	uint32_t QueueFamilyIndex;
};

class VulkanInternalSyncResource : RenderResourceInternal
{
public:
	virtual ~VulkanInternalSyncResource() {}

	ResourceState InternalResourceState;

	concurrency::concurrent_queue<std::pair<vk::Semaphore, uint32_t>> ComputeTransferOps;
	int32_t SyncPoint;
	std::shared_mutex SyncPointCreation;
	RenderTaskInternal *Owner;
};

class ConcurrentResourceUsage
{
public:
	VulkanInternalSyncResource *Resource;
	ResourceBarrierType BarrierType;
	ResourceState BeginState;
	ResourceState EndState;
};

class VulkanGraphicsPipeline : public VulkanInternalSyncResource
{
public:
	VulkanGraphicsPipeline(RenderGraphInstance *inst, GraphicsExecutionStateData& data);
	virtual ~VulkanGraphicsPipeline() override;
	vk::PipelineLayout GetLayout();
	ShaderPassInterfaceState& GetShaderPassInterface();
	vk::Pipeline Compile(vk::RenderPass renderPass, uint32_t subPass);
private:
	RenderGraphInstance *m_inst;
	ShaderPassInterfaceState m_shaderPassInterface;
	vk::PipelineLayout m_pipelineLayout;
	std::map<std::tuple<vk::RenderPass, uint32_t>, vk::Pipeline> m_pipes;
};

class VulkanComputePipeline : public VulkanInternalSyncResource
{
public:
	VulkanComputePipeline(RenderGraphInstance *inst, ComputeExecutionStateData& data);
	virtual ~VulkanComputePipeline() override;
	vk::PipelineLayout GetLayout();
	ShaderPassInterfaceState& GetShaderPassInterface();
	vk::Pipeline Get();
private:
	ShaderPassInterfaceState m_shaderPassInterface;
	vk::PipelineLayout m_pipelineLayout;
	vk::Pipeline m_pipe;
};

class VulkanQueryPool : public VulkanInternalSyncResource
{
public:
	VulkanQueryPool(RenderGraphInstance *inst, QueryType type, int32_t count, PipelineStatisticBits statistics);
	virtual ~VulkanQueryPool() override;
	vk::QueryPool GetQueryPool();
private:
	vk::QueryPool m_queryPool;
};

class VulkanBuffer : public VulkanInternalSyncResource
{
public:
	VulkanBuffer(RenderGraphInstance *inst, uint64_t size, bool frequentUpdates, BufferUsageBit usage);
	virtual ~VulkanBuffer() override;
	vk::Buffer GetBuffer();
	uint64_t GetSize();
	bool IsMapped();
	void *GetMappedPointer();
private:
	vk::Buffer m_buffer;
	uint64_t m_size;
};

class VulkanDescriptorSetLayout : public VulkanInternalSyncResource
{
public:
	VulkanDescriptorSetLayout(RenderGraphInstance *inst, ShaderViewCollectionData& data);
	virtual ~VulkanDescriptorSetLayout() override;
	vk::DescriptorSetLayout GetLayout();
	ShaderViewCollectionData& GetData();
private:
	ShaderViewCollectionData m_data;
	vk::DescriptorSetLayout m_layout;
};

class VulkanDescriptorSet : public VulkanInternalSyncResource
{
public:
	VulkanDescriptorSet(RenderGraphInstance *inst, VulkanDescriptorSetLayout *layout);
	virtual ~VulkanDescriptorSet() override;
	vk::DescriptorSet GetSet();
	VulkanDescriptorSetLayout *GetDescriptorSetLayout();
private:
	VulkanDescriptorSetLayout *m_layout;
	vk::DescriptorSet m_set;
};

class VulkanImage : public VulkanInternalSyncResource
{
public:
	VulkanImage(RenderGraphInstance *inst, ImageType type, glm::ivec3 size, int32_t levels, int32_t layers, RenderFormat format, ImageUsageBit usage);
	virtual ~VulkanImage() override;
	vk::Image GetImage();
	ImageType GetType();
	glm::ivec3 GetSize();
	int32_t GetLevels();
	int32_t GetLayers();
	RenderFormat GetFormat();
	vk::ImageView GetSubresource(ImageSubresource subresource);
private:
	std::map<ImageSubresource, vk::ImageView> m_subresources;
	vk::Image m_image;
	ImageType m_type;
	glm::ivec3 m_size;
	int32_t m_levels;
	int32_t m_layers;
	RenderFormat m_format;
};

class VulkanSampler : public VulkanInternalSyncResource
{
public:
	VulkanSampler(RenderGraphInstance *inst, SamplerState state);
	virtual ~VulkanSampler() override;
	vk::Sampler GetSampler();
private:
	vk::Sampler m_sampler;
};

class VulkanSpecializedShaderModule : public VulkanInternalSyncResource
{
public:
	VulkanSpecializedShaderModule(RenderGraphInstance *inst, ShaderStageBit stage, std::vector<char> spirvCode,
		std::vector<VulkanSpecializationConstantDataDescription> constantDescriptions, void *constantData, uint32_t constDataSize);
	virtual ~VulkanSpecializedShaderModule() override;
	vk::ShaderModule GetModule();
	vk::SpecializationInfo& GetSpecializationInfo();
	void *GetSpecializationData();
private:
	vk::ShaderModule m_module;
	vk::SpecializationInfo m_specializationInfo;
	void *m_data;
	uint32_t m_dataSize;
};

class VulkanRenderTargetPattern
{
public:
	std::vector<RenderResource> Images;
	std::vector<RenderPassAttachmentDescription> Descriptions;
	std::vector<ImageSubresource> Subresources;
	RenderResource DepthStencil;
	ImageSubresource DepthStencilSubresource;
};

class VulkanRenderTargetCache
{
public:
	VulkanRenderTargetCache(RenderGraphInstance *inst);
	~VulkanRenderTargetCache();
	std::pair<vk::Framebuffer, vk::RenderPass> GetRenderTarget(VulkanRenderTargetPattern pattern);
private:
	std::map<VulkanRenderTargetPattern, std::pair<vk::Framebuffer, vk::RenderPass>> m_rts;
	std::map<VulkanRenderTargetPattern, std::vector<vk::ImageView>> m_imageViews;
};

class TransferReadTask
{
public:
	uint64_t RingBufferOffset;
	void *Destination;
	uint64_t Size;
};

class VulkanComputeTask : public ComputeTaskInternal
{
public:
	vk::Semaphore Semaphore;
	virtual ~VulkanComputeTask() override;
	virtual bool IsDone() override;
};

class VulkanTransferTask : public TransferTaskInternal
{
public:
	vk::Semaphore Semaphore;
	std::vector<TransferReadTask> TransferReadTasks;
	virtual ~VulkanTransferTask() override;
	virtual bool IsDone() override;
};

class RenderResourceInternalState
{
public:
	bool TransitionedYet;
	ResourceState InitialState;
	ResourceState CurrentState;
	int32_t SyncPoint;
};

class VulkanInternalCommandListStateTracker
{
public:
	VulkanInternalCommandListStateTracker(RenderGraphInstance *instance);
	std::shared_ptr<VulkanComputeTask> BeginComputeTask();
	std::shared_ptr<VulkanTransferTask> BeginTransferTask();
	void BeginNewCommandBuffer();
	void BindGraphicsPipeline(VulkanGraphicsPipeline *pipeline);
	void BindComputePipeline(VulkanComputePipeline *pipeline);
	void SVBindingArrayDependencyHint(uint32_t svCollectionIndex, uint32_t binding, uint32_t first, uint32_t count);
	void BeginRenderPass(RenderPassBeginInfo info);
	void EndRenderPass();
	void TransitionResource(RenderResource resource, ResourceState& newState, bool now);
	void InsertPipelineBarriers();
	vk::PipelineStageFlagBits WritePipelineStage(vk::PipelineStageFlagBits bits);
	vk::PipelineStageFlagBits GetCurrentPipeStage();
	vk::CommandBuffer& GetCommandBuffer();
	vk::PipelineBindPoint GetBindPoint();
	vk::PipelineLayout& GetCurrentPipeLayout();
	void SubmitCommandBufferAndReset();
private:
	RenderGraphInstance *m_renderGraph;
	vk::CommandBuffer m_cmdBuffer = nullptr;
	vk::PipelineStageFlagBits m_stage = vk::PipelineStageFlagBits::eTopOfPipe;
	uint32_t m_queueFamily;
	bool m_computerPipeline;
	bool m_transferOperation;
	std::shared_ptr<VulkanComputeTask> m_curComputeTask;
	std::shared_ptr<VulkanTransferTask> m_curTransferTask;
	VulkanGraphicsPipeline *m_usedGraphicsPipeline;
	VulkanComputePipeline *m_usedComputePipeline;
	std::unordered_map<uint64_t, std::vector<uint64_t>> m_svArrayHints;
	std::map<RenderResource, RenderResourceInternalState> m_resourcesTracking;
	VulkanRenderTargetCache m_pCache;
	VulkanSpecializedShaderModule m_smCache;
};

class RenderGraphNode
{
public:
	std::vector<RenderGraphNode *> Inputs;
	std::vector<RenderGraphNode *> Outputs;
	ISystem *System;
	RenderTaskInternal *Task;
};

class RenderGraphInstance
{
public:
	RenderGraphInstance(void *vkInstancePointer, void *vkSurfacePointer);
	void SetSystemGraphSorter(SystemGraphSorter *sysGraph);
	RenderTask *GetRenderTask(ISystem *system);
	uint32_t GetFrameImageCount();
	uint32_t GetCurrentFrameImageIndex();
	vk::Buffer AllocateBuffer(uint64_t size, bool frequentUpdates, BufferUsageBit usage);
	vk::Image AllocateImage(ImageType type, glm::ivec3 size, int32_t levels, int32_t layers, RenderFormat format, ImageUsageBit usage);
	void FreeBuffer(vk::Buffer buffer);
	void FreeImage(vk::Image image);
	std::pair<vk::Buffer, uint64_t> AllocateRingBufferSpot(uint64_t size, vk::Semaphore semaphore, uint64_t waitValue);
	void *GetRingBufferPointer();
	vk::Instance GetInstance();
	vk::Device GetDevice();
	uint32_t GetGraphicsQueueFamily();
	uint32_t GetComputeQueueFamily();
	uint32_t GetTransferQueueFamily();
	int32_t CreateSyncPoint();
	void SendSecondaryCommandBuffer(int32_t syncPoint, std::vector<ConcurrentResourceUsage> usage, vk::CommandBuffer buf,
		bool compute, bool transfer, vk::Semaphore semaphore, uint64_t waitValue);
private:

};

class RenderTaskInternal
{
public:
	RenderGraphInstance *GraphInstance;
	VulkanInternalCommandListStateTracker CommandListStateTracker;
};