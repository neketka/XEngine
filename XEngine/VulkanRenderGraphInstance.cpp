#include "pch.h"
#include "VulkanRenderGraphInstance.h"

RenderTask::~RenderTask()
{
}

RenderTask::RenderTask(RenderTaskInternal *renderTask, RenderGraphInstance *graphInstance) : m_renderTask(renderTask),
	m_graphInstance(graphInstance)
{
}

uint32_t RenderTask::GetFrameImageCount()
{
	return m_graphInstance->GetFrameImageCount();
}

uint32_t RenderTask::GetCurrentFrameImageIndex()
{
	return m_graphInstance->GetCurrentFrameImageIndex();
}

RenderResource RenderTask::CreateGraphicsExecutionState(GraphicsExecutionStateData data)
{
	return RenderResource(std::static_pointer_cast<RenderResourceInternal>
		(std::make_shared<VulkanGraphicsPipeline>(m_renderTask, data)), 
		RenderResourceType::GraphicsExecutionState);
}

RenderResource RenderTask::CreateComputeExecutionState(ComputeExecutionStateData data)
{
	return RenderResource(std::static_pointer_cast<RenderResourceInternal>
		(std::make_shared<VulkanComputePipeline>(m_renderTask, data)),
		RenderResourceType::ComputeExecutionState);
}

RenderResource RenderTask::CreateBuffer(uint64_t size, bool frequentUpdates, BufferUsageBit usage)
{
	return RenderResource(std::static_pointer_cast<RenderResourceInternal>
		(std::make_shared<VulkanBuffer>(m_graphInstance, size, frequentUpdates, usage)), RenderResourceType::Buffer);
}

RenderResource RenderTask::CreateImage(ImageType type, glm::ivec3 size, int32_t levels, int32_t layers, RenderFormat format, ImageUsageBit usage)
{
	return RenderResource(std::static_pointer_cast<RenderResourceInternal>
		(std::make_shared<VulkanImage>(m_graphInstance, type, size, levels, layers, format, usage)), 
		RenderResourceType::Image);
}

RenderResource RenderTask::CreateSampler(SamplerState state)
{
	return RenderResource(std::static_pointer_cast<RenderResourceInternal>
		(std::make_shared<VulkanSampler>(m_graphInstance, state)), RenderResourceType::Sampler);
}

RenderResource RenderTask::CreateShaderViewCollectionLayout(ShaderViewCollectionData data)
{
	return RenderResource(std::static_pointer_cast<RenderResourceInternal>
		(std::make_shared<VulkanDescriptorSetLayout>(m_graphInstance, data)),
		RenderResourceType::ShaderViewCollectionLayout);
}

RenderResource RenderTask::CreateShaderViewCollection(RenderResource layout)
{
	return RenderResource(std::static_pointer_cast<RenderResourceInternal>
		(std::make_shared<VulkanDescriptorSet>(m_graphInstance, 
			dynamic_cast<VulkanDescriptorSetLayout *>(layout.m_handle.get()))), 
		RenderResourceType::ShaderViewCollection);
}

RenderResource RenderTask::CreateQuerySet(QueryType type, int32_t count, PipelineStatisticBits statistics)
{
	return RenderResource(std::static_pointer_cast<RenderResourceInternal>
		(std::make_shared<VulkanQueryPool>(m_graphInstance, type, count, statistics)),
		RenderResourceType::QuerySet);
}

RenderResource RenderTask::CreateVulkanShader(ShaderStageBit stage, std::vector<char> spirvCode, std::vector<VulkanSpecializationConstantDataDescription> constantDescriptions, void *constantData, uint32_t constDataSize)
{
	return RenderResource(std::static_pointer_cast<RenderResourceInternal>
		(std::make_shared<VulkanSpecializedShaderModule>(m_graphInstance, stage, spirvCode, constantDescriptions, constantData, constDataSize)), 
		RenderResourceType::Shader);
}

void RenderTask::BeginTask()
{
	m_renderTask->CommandListStateTracker.BeginNewCommandBuffer();
}

ComputeTask RenderTask::BeginComputeTask()
{
	return ComputeTask(m_renderTask->CommandListStateTracker.BeginComputeTask());
}

TransferTask RenderTask::BeginTransferTask()
{
	return TransferTask(m_renderTask->CommandListStateTracker.BeginTransferTask());
}

void RenderTask::EndTask()
{
	m_renderTask->CommandListStateTracker.SubmitCommandBufferAndReset();
}

void RenderTask::BeginQuery(RenderResource querySet, int32_t index)
{
	VulkanQueryPool *qPool = dynamic_cast<VulkanQueryPool *>(querySet.m_handle.get());
	m_renderTask->CommandListStateTracker.GetCommandBuffer().beginQuery(qPool->GetQueryPool(), index, vk::QueryControlFlags(0));
}

void RenderTask::EndQuery(RenderResource querySet, int32_t index)
{
	VulkanQueryPool *qPool = dynamic_cast<VulkanQueryPool *>(querySet.m_handle.get());
	m_renderTask->CommandListStateTracker.GetCommandBuffer().endQuery(qPool->GetQueryPool(), index);
}

void RenderTask::ResetQueryRange(RenderResource querySet, int32_t index, int32_t count)
{
	VulkanQueryPool *qPool = dynamic_cast<VulkanQueryPool *>(querySet.m_handle.get());
	m_renderTask->CommandListStateTracker.GetCommandBuffer().resetQueryPool(qPool->GetQueryPool(), index, count);
}

void RenderTask::WriteTimestamp(RenderResource querySet, int32_t index)
{
	VulkanQueryPool *qPool = dynamic_cast<VulkanQueryPool *>(querySet.m_handle.get());
	m_renderTask->CommandListStateTracker.GetCommandBuffer().writeTimestamp(m_renderTask
		->CommandListStateTracker.GetCurrentPipeStage(), qPool->GetQueryPool(), index);
}

void RenderTask::BindShaderViewCollection(RenderResource collection, uint32_t dynamicOffset, uint32_t index)
{
	VulkanDescriptorSet *dSet = dynamic_cast<VulkanDescriptorSet *>(collection.m_handle.get());
	m_renderTask->CommandListStateTracker.GetCommandBuffer().bindDescriptorSets(
		m_renderTask->CommandListStateTracker.GetBindPoint(),
		m_renderTask->CommandListStateTracker.GetCurrentPipeLayout(),
		index, { dSet->GetSet() }, { dynamicOffset }
	);
}

void RenderTask::UpdateShaderViewCollectionImage(RenderResource shaderViewCollection, uint32_t bindingIndex, uint32_t firstArrayIndex, std::vector<ImageShaderView> images)
{
	VulkanDescriptorSet *dSet = dynamic_cast<VulkanDescriptorSet *>(shaderViewCollection.m_handle.get());
	
	ShaderViewType type = dSet->GetDescriptorSetLayout()->GetData().Views[bindingIndex].Type;
	std::vector<vk::DescriptorImageInfo> imageInfos(images.size());
	vk::DescriptorType dType;

	if (type == ShaderViewType::SamplingImage || type == ShaderViewType::InputImage)
	{
		for (int32_t i = 0; i < images.size(); ++i)
		{
			ImageShaderView& srv = images[i];
			VulkanImage *img = dynamic_cast<VulkanImage *>(srv.Image.m_handle.get());
			vk::DescriptorImageInfo& imgInfo = imageInfos[i];
			imgInfo.setImageView(img->GetSubresource(srv.ImageSubresource));
			imgInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
		}
		if (type == ShaderViewType::InputImage)
			dType = vk::DescriptorType::eInputAttachment;
		else
			dType = vk::DescriptorType::eSampledImage;
	}
	else if (type == ShaderViewType::StorageImage)
	{
		for (int32_t i = 0; i < images.size(); ++i)
		{
			ImageShaderView& srv = images[i];
			VulkanImage *img = dynamic_cast<VulkanImage *>(srv.Image.m_handle.get());
			vk::DescriptorImageInfo& imgInfo = imageInfos[i];
			imgInfo.setImageView(img->GetSubresource(srv.ImageSubresource));
			imgInfo.setImageLayout(vk::ImageLayout::eGeneral);
		}
		dType = vk::DescriptorType::eStorageImage;
	}
	else if (type == ShaderViewType::Sampler)
	{
		for (int32_t i = 0; i < images.size(); ++i)
		{
			ImageShaderView& srv = images[i];
			VulkanSampler *smpl = dynamic_cast<VulkanSampler *>(srv.Sampler.m_handle.get());
			vk::DescriptorImageInfo& imgInfo = imageInfos[i];
			imgInfo.setSampler(smpl->GetSampler());
		}
		dType = vk::DescriptorType::eSampler;
	}

	vk::WriteDescriptorSet write;
	write.setPImageInfo(imageInfos.data());
	write.setDescriptorType(dType);
	write.setDescriptorCount(imageInfos.size());
	write.setDstSet(dSet->GetSet());
	write.setDstBinding(bindingIndex);
	write.setDstArrayElement(firstArrayIndex);
	
	m_graphInstance->GetDevice().updateDescriptorSets({ write }, {});
}

void RenderTask::UpdateShaderViewCollectionBuffer(RenderResource shaderViewCollection, uint32_t bindingIndex, uint32_t firstArrayIndex, std::vector<BufferShaderView> buffers)
{
	VulkanDescriptorSet *dSet = dynamic_cast<VulkanDescriptorSet *>(shaderViewCollection.m_handle.get());

	ShaderViewType type = dSet->GetDescriptorSetLayout()->GetData().Views[bindingIndex].Type;

	vk::DescriptorType dType = type == ShaderViewType::ConstantBuffer ? vk::DescriptorType::eUniformBuffer :
		type == ShaderViewType::DynamicOffsetConstantBuffer ? vk::DescriptorType::eUniformBufferDynamic :
		type == ShaderViewType::StorageBuffer ? vk::DescriptorType::eStorageBuffer : vk::DescriptorType::eStorageBufferDynamic;

	std::vector<vk::DescriptorBufferInfo> bufferInfos(buffers.size());

	for (int32_t i = 0; i < bufferInfos.size(); ++i)
	{
		BufferShaderView& srv = buffers[i];
		VulkanBuffer *buf = dynamic_cast<VulkanBuffer *>(srv.Buffer.m_handle.get());
		vk::DescriptorBufferInfo& bufInfo = bufferInfos[i];
		bufInfo.setBuffer(buf->GetBuffer());
		bufInfo.setOffset(srv.BufferSubresource.Offset);
		bufInfo.setRange(srv.BufferSubresource.Size);
	}

	vk::WriteDescriptorSet write;
	write.setPBufferInfo(bufferInfos.data());
	write.setDescriptorType(dType);
	write.setDescriptorCount(bufferInfos.size());
	write.setDstSet(dSet->GetSet());
	write.setDstBinding(bindingIndex);
	write.setDstArrayElement(firstArrayIndex);

	m_graphInstance->GetDevice().updateDescriptorSets({ write }, {});
}

void RenderTask::PushShaderConstants(ShaderStageBit stages, uint32_t offset, uint32_t size, void *data)
{
}

void RenderTask::PushMarker(glm::vec4 color, std::string name)
{
}

void RenderTask::PopMarker()
{
}

void RenderTask::DeclareShaderViewArrayUsage(uint32_t svCollectionIndex, uint32_t binding, uint32_t first, uint32_t count)
{
}

void RenderTask::BindComputeExecutionState(RenderResource state)
{
}

void RenderTask::Dispatch(int32_t x, int32_t y, int32_t z)
{
}

void RenderTask::DispatchIndirect(RenderResource indirectBuffer, uint64_t bufferOffset)
{
}

void RenderTask::BindGraphicsExecutionState(RenderResource state)
{
}

void RenderTask::UpdateViewport(glm::ivec2 offset, glm::ivec2 size)
{
}

void RenderTask::UpdateScissorBox(glm::ivec2 offset, glm::ivec2 size)
{
}

void RenderTask::BindVertexBuffer(RenderResource vertexBuffer, uint64_t bufferOffset, uint32_t binding)
{
}

void RenderTask::BindIndexBuffer(RenderResource indexBuffer, uint64_t bufferOffset, bool shortIndexType)
{
}

void RenderTask::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t baseInstance)
{
}

void RenderTask::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t baseInstance)
{
}

void RenderTask::DrawIndirect(RenderResource indirectBuffer, uint64_t bufferOffset, uint32_t drawCount, uint32_t stride)
{
}

void RenderTask::DrawIndexedIndirect(RenderResource indirectBuffer, uint64_t bufferOffset, uint32_t drawCount, uint32_t stride)
{
}

void RenderTask::DrawIndirectCount(RenderResource indirectBuffer, uint64_t indirectBufferOffset, RenderResource countBuffer, uint64_t countBufferOffset, uint32_t maxDrawCount, uint32_t indirectStride)
{
}

void RenderTask::DrawIndexedIndirectCount(RenderResource indirectBuffer, uint64_t indirectBufferOffset, RenderResource countBuffer, uint64_t countBufferOffset, uint32_t maxDrawCount, uint32_t indirectStride)
{
}

void RenderTask::BeginRenderPass(RenderPassBeginInfo info)
{
}

void RenderTask::EndRenderPass()
{
}

void RenderTask::WriteQueryToBuffer(RenderResource querySet, RenderResource buffer, uint64_t bufferOffset)
{
}

void RenderTask::CopyImage(RenderResource src, RenderResource dest, std::vector<ImageCopyOperation> copyRegions)
{
}

void RenderTask::CopyBuffer(RenderResource src, RenderResource dest, uint64_t srcOffset, uint64_t destOffset, uint64_t size)
{
}

void RenderTask::UpdateBuffer(RenderResource buffer, uint64_t offset, uint32_t size, void * data)
{
}

void RenderTask::UpdateImage(RenderResource image, ImageSubresourceLevel subResource, void * data)
{
}

void RenderTask::ReadBuffer(RenderResource buffer, uint64_t offset, uint32_t size, void * dest)
{
}

void RenderTask::ReadImage(RenderResource image, ImageSubresourceLevel subResource, RenderFormat dataFormat, void * dest)
{
}

bool VulkanComputeTask::IsDone()
{
	return false;
}

bool VulkanTransferTask::IsDone()
{
	return false;
}
