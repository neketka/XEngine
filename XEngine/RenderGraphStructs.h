#pragma once

#include "RenderGraphEnum.h"

class SamplerState
{
public:
	bool UnnormalizedCoordinates;
	SamplerFilteringMode Magnification;
	SamplerFilteringMode Minification;
	SamplerFilteringMode Mipmapping;
	SamplerWrapMode WrapU;
	SamplerWrapMode WrapV;
	SamplerWrapMode WrapW;
	float LODBias;
	bool AnisotropicFiltering;
	float MaxAnisotropy;
	bool DepthComparison;
	ComparisonMode DepthComparisonMode;
	float MinLOD;
	float MaxLOD;
	glm::vec4 BorderColor;
	bool UseBorderAsInteger;
}; 

class RenderVertexBufferInputData
{
public:
	int32_t BufferBinding;
	int32_t Stride;
	bool Instanced;
};

class RenderVertexAttributeData
{
public:
	int32_t ShaderAttributeLocation;
	int32_t BufferBinding;
	RenderFormat Format;
	int32_t InElementOffset;
};

class RenderVertexInputState
{
public:
	PrimitiveType Topology;
	bool PrimitiveRestart;
	std::vector<RenderVertexBufferInputData> BufferData;
	std::vector<RenderVertexAttributeData> AttributeData;
};

class RenderTessellationState
{
public:
	bool EnableState;
	int32_t PatchControlPoints;
};

class RenderViewportState
{
public:
	glm::ivec4 ViewportBounds;
	glm::ivec4 ScissorBounds;
	bool ViewportDynamic;
	bool ScissorDynamic;
};

class RenderRasterState
{
public:
	bool EnableState;

	bool DoDepthClamp;
	bool RasterizerDiscard;
	PolygonMode DrawingMode;
	bool CullFrontFace;
	bool CullBackFace;
	bool LineWidthDynamic;
	bool ClockwiseFrontFace;
	float LineWidth;
};

class RenderStencilOperationState
{
public:
	StencilOperation Fail;
	StencilOperation Pass;
	StencilOperation DepthFail;
	ComparisonMode CompareOperation;
	uint32_t CompareMask;
	uint32_t WriteMask;
	uint32_t Reference;
	bool DynamicCompareMask;
	bool DynamicWriteMask;
	bool DynamicReference;
};

class RenderDepthStencilState
{
public:
	bool EnableState;

	bool DepthTest;
	bool DepthWrite;
	ComparisonMode DepthCompareOp;
	bool StencilTest;
	RenderStencilOperationState StencilFront;
	RenderStencilOperationState StencilBack;
};

class RenderBlendingAttachmentState
{
public:
	bool Blending;
	BlendFactor SrcColorBlendFactor;
	BlendFactor DestColorBlendFactor;
	BlendingOperation ColorBlendOperation;
	BlendFactor SrcAlphaBlendFactor;
	BlendFactor DestAlphaBlendFactor;
	BlendingOperation AlphaBlendOperation;
	glm::bvec4 ColorWriteMask;
};

class RenderBlendingState
{
public:
	bool DoLogicOperations;
	BlendLogicOperation LogicOperation;
	std::vector<RenderBlendingAttachmentState> BufferBlending;
	glm::vec4 BlendConstants;
};

class BufferSubresource
{
public:
	uint32_t Offset;
	uint32_t Size;
};

class ImageSubresource
{
public:
	bool Color;
	bool Depth;
	bool Stencil;
	uint32_t BaseMipLevel;
	uint32_t LevelCount;
	uint32_t BaseArrayLayer;
	uint32_t LayerCount;
};

class ImageSubresourceLevel
{
public:
	bool Color;
	bool Depth;
	bool Stencil;
	uint32_t MipLevel;
	uint32_t BaseArrayLayer;
	uint32_t LayerCount;
};

class ImageCopyOperation
{
public:
	ImageSubresourceLevel SrcSubresource;
	ImageSubresourceLevel DestSubresource;
	glm::ivec3 SrcOffset;
	glm::ivec3 DestOffset;
	glm::ivec3 Size;
};

class RenderTask;

class RenderResourceInternal
{
public:
	virtual ~RenderResourceInternal() {}
	RenderTask *Owner;
};

class RenderResource
{
public:
	RenderResource() {}
	void FreeReference()
	{
		m_handle = nullptr;
	}
	bool operator==(const RenderResource& other)
	{
		return other.m_handle.get() == m_handle.get();
	}
	const static RenderResource NullResource;
private:
	RenderResource(std::shared_ptr<RenderResourceInternal> handle, RenderResourceType type) : m_handle(handle), m_type(type) {}
	friend class RenderGraphInstance;
	friend class RenderTask;
	std::shared_ptr<RenderResourceInternal> m_handle = nullptr;
	RenderResourceType m_type = RenderResourceType::NullType;
};

class ShaderViewData
{
public:
	int Binding;
	int Count;
	ShaderViewType Type;
	ShaderStageBit ShaderStages;
};

class ShaderViewCollectionData
{
public:
	std::vector<ShaderViewData> Views;
};

class RenderPassAttachmentDescription
{
public:
	bool PreserveContentsAtBeginning;
	bool PreserveContentsAtEnd;
	bool ClearAttachment;
};

union ColorClear
{
public:
	glm::ivec4 IntColor;
	glm::vec4 FloatColor;
};

class DepthStencilClear
{
public:
	float Depth;
	uint32_t Stencil;
};

union ClearValue
{
public:
	ColorClear Color;
	DepthStencilClear DepthStencil;
};

class RenderPassBeginInfo
{
public:
	std::vector<RenderResource> Images; 
	std::vector<ClearValue> ClearValues;
	std::vector<RenderPassAttachmentDescription> Descriptions;
	std::vector<ImageSubresource> Subresources;
	RenderResource DepthStencil;
	ImageSubresource DepthStencilSubresource;
};

class ExecutionDependencyInfo
{
public:
	std::vector<RenderResource> Inputs;
	std::vector<ExecutionDependencyBits> InDependencies;
	std::vector<RenderResource> Outputs;
	std::vector<ExecutionDependencyBits> OutDependencies;
};

class ShaderConstantRange 
{
public:
	ShaderStageBit ShaderStages;
	uint32_t Offset;
	uint32_t Size;
};

class ShaderPassInterfaceState
{
public:
	std::vector<RenderResource> ShaderViewCollections;
	std::vector<ShaderConstantRange> ShaderConstants;
};

class GraphicsExecutionStateData
{
public:
	std::vector<RenderResource> Shaders;
	ShaderPassInterfaceState ShaderPassInterface;

	RenderTessellationState RenderTessellationState;
	RenderViewportState ViewportState;
	RenderRasterState RasterState;
	RenderDepthStencilState DepthStencilState;
	RenderVertexInputState VertexInputState;
	RenderBlendingState BlendingState;
};

class ComputeExecutionStateData
{
public:
	std::vector<RenderResource> Shaders;
	ShaderPassInterfaceState ShaderInterface;
};

class VulkanSpecializationConstantDataDescription
{
public:
	uint32_t ConstantId;
	uint32_t DataOffset;
	uint32_t DataSize;
};

class ComputeTaskInternal
{
public:
	virtual ~ComputeTaskInternal() {}
	virtual bool IsDone() = 0;
};

class TransferTaskInternal
{
public:
	virtual ~TransferTaskInternal() {}
	virtual bool IsDone() = 0;
};

class TransferTask
{
public:
	TransferTask(std::shared_ptr<TransferTaskInternal> handle) : m_handle(handle) {}
	bool IsDone()
	{
		return m_handle->IsDone();
	}
private:
	std::shared_ptr<TransferTaskInternal> m_handle;
};

class ComputeTask
{
public:
	ComputeTask(std::shared_ptr<ComputeTaskInternal> handle) : m_handle(handle) {}
	bool IsDone()
	{
		return m_handle->IsDone();
	}
private:
	std::shared_ptr<ComputeTaskInternal> m_handle;
};

class ImageShaderView
{
public:
	RenderResource Sampler;
	RenderResource Image;
	ImageSubresource ImageSubresource;
};

class BufferShaderView 
{
public:
	RenderResource Buffer;
	BufferSubresource BufferSubresource;
};