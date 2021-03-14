#pragma once
#include "GraphicsEnums.h"
#include <vector>
#include <glm/glm.hpp>
#include <string>

class DisposableHandle
{
public:
	virtual ~DisposableHandle() = 0;
};

enum class GraphicsIdentity
{
	OpenGL4_6, Vulkan1_2
};

using GraphicsSpecificSpecializationData = DisposableHandle;
using GraphicsSpecificShaderCode = DisposableHandle;

using GraphicsRenderPipeline = DisposableHandle;
using GraphicsComputePipeline = DisposableHandle;
using GraphicsRenderPass = DisposableHandle;
using GraphicsShaderDataSet = DisposableHandle;
using GraphicsShaderResourceInstance = DisposableHandle;
using GraphicsSampler = DisposableHandle;

using GraphicsShader = DisposableHandle;
using GraphicsRenderTarget = DisposableHandle;

using GraphicsQuery = DisposableHandle;

using GraphicsImageView = DisposableHandle;

class GraphicsSpecificStructure
{
public:
	virtual std::string GetApiString() = 0;
};

class GraphicsSyncObject
{
public:
	virtual ~GraphicsSyncObject() = 0;
	virtual bool Wait(uint64_t nanoSecTimeout) = 0;
	virtual void Reset() = 0;
	virtual bool GetCurrentStatus() = 0;
};

class GraphicsImageObject
{
public:
	virtual ~GraphicsImageObject() = 0;
	virtual GraphicsImageView *CreateView(ImageType type, VectorDataFormat format, int32_t baseLevel, int32_t levels, int32_t baseLayer, int32_t layers,
		ImageSwizzleComponent redSwizzle, ImageSwizzleComponent greenSizzle, ImageSwizzleComponent blueSwizzle,
		ImageSwizzleComponent alphaSwizzle) = 0;
};

class GraphicsShaderConstantData
{
public:
	ShaderStageBit Stages;
	int32_t Offset;
	int32_t ElementCount;
	bool Float;
	bool Matrix;
	bool Unsigned;
	int32_t VectorLength;
};

class GraphicsShaderResourceViewData
{
public:
	ShaderStageBit Stages;
	int32_t Binding;
	ShaderResourceType Type;
	int32_t Count;
};

class GraphicsSamplerState
{
public:
	bool EnableMipmapping;
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

class GraphicsMemoryBuffer
{
public:
	virtual ~GraphicsMemoryBuffer() = 0;
	virtual void MapBuffer(uint64_t offset, int32_t length, bool coherent, bool writeOnly) = 0;
	virtual void UnmapBuffer() = 0;
	virtual void *GetMappedPointer() = 0;
	virtual void FlushMapped(uint64_t offset, int32_t size) = 0;
	virtual void InvalidateMapped(uint64_t offset, int32_t size) = 0;
};

class GraphicsShaderState
{
public:
	GraphicsShader *Shader;
	GraphicsSpecificSpecializationData *Specialization;
};

class GraphicsRenderSubpassState
{
public:
	std::vector<int32_t> InputAttachments;
	std::vector<int32_t> ColorAttachments;
};

class GraphicsRenderPassState
{
public:
	std::vector<GraphicsRenderSubpassState> Subpasses;
	std::vector<bool> ReadOnlyAttachments;
	std::vector<GraphicsRenderPassLoadStore> LoadOperations;
	std::vector<GraphicsRenderPassLoadStore> StoreOperations;
	GraphicsRenderPassLoadStore StencilLoadOperation;
	GraphicsRenderPassLoadStore StencilStoreOperation;
};

class GraphicsRenderVertexBufferInputData
{
public:
	int32_t BufferBinding;
	int32_t Stride;
	bool Instanced;
};

class GraphicsRenderVertexAttributeData
{
public:
	int32_t ShaderAttributeLocation;
	int32_t BufferBinding;
	VectorDataFormat Format;
	int32_t InElementOffset;
};

class GraphicsRenderVertexInputState
{
public:
	GraphicsPrimitiveType Topology;
	bool PrimitiveRestart;
	std::vector<GraphicsRenderVertexBufferInputData> BufferData;
	std::vector<GraphicsRenderVertexAttributeData> AttributeData;
};

class GraphicsRenderTessellationState
{
public:
	bool EnableState;
	int32_t PatchControlPoints;
};

class GraphicsRenderViewportState
{
public:
	glm::ivec4 ViewportBounds;
	glm::ivec4 ScissorBounds;
	bool ViewportDynamic;
	bool ScissorDynamic;
};

class GraphicsRenderRasterState
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

class GraphicsRenderStencilOperationState
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

class GraphicsRenderDepthStencilState
{
public:
	bool EnableState;

	bool DepthTest;
	bool DepthWrite;
	ComparisonMode DepthCompareOp;
	bool StencilTest;
	GraphicsRenderStencilOperationState StencilFront;
	GraphicsRenderStencilOperationState StencilBack;
};

class GraphicsRenderBlendingAttachmentState
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

class GraphicsRenderBlendingState
{
public:
	bool DoLogicOperations;
	BlendLogicOperation LogicOperation;
	std::vector<GraphicsRenderBlendingAttachmentState> BufferBlending;
	glm::vec4 BlendConstants;
};

class GraphicsRenderPipelineState
{
public:
	std::vector<GraphicsShaderState> Shaders;
	GraphicsShaderDataSet *ShaderDataSet;
	GraphicsRenderTessellationState RenderTessellationState;
	GraphicsRenderViewportState ViewportState;
	GraphicsRenderRasterState RasterState;
	GraphicsRenderDepthStencilState DepthStencilState;
	GraphicsRenderVertexInputState VertexInputState;
	GraphicsRenderBlendingState BlendingState;
};

class GraphicsComputePipelineState
{
public:
	GraphicsShaderDataSet *ShaderDataSet;
	GraphicsShaderState ShaderState;
};

class GraphicsBufferImageCopyRegion
{
public:
	glm::ivec3 Offset; 
	glm::ivec3 Size;
	int32_t Level;
	int32_t BufferOffset; 
	int32_t BufferRowLength; 
	int32_t BufferImageHeight;
};

class GraphicsCommandBuffer : public DisposableHandle
{
public:
	virtual void BindRenderPipeline(GraphicsRenderPipeline *pipeline) = 0;
	virtual void BindComputePipeline(GraphicsComputePipeline *pipeline) = 0;

	virtual void BindRenderPass(GraphicsRenderTarget *target, GraphicsRenderPass *renderPass, std::vector<glm::vec4>&& attachmentClearValues, char stencil) = 0;
	virtual void NextSubpass() = 0;
	virtual void EndRenderPass() = 0;

	virtual void BeginQuery(GraphicsQuery *query) = 0;
	virtual void EndQuery(GraphicsQuery *query) = 0;
	virtual void ResetQuery(GraphicsQuery *query) = 0;
	virtual void WriteTimestamp(GraphicsQuery *query) = 0;
	virtual void WriteQueryToBuffer(GraphicsQuery *query, int32_t bufferOffset) = 0;

	virtual void BindVertexBuffers(int32_t firstBinding, std::vector<GraphicsMemoryBuffer *> buffers, std::vector<int32_t> offsets) = 0;
	virtual void BindIndexBuffer(GraphicsMemoryBuffer *buffer, bool dataType16bit) = 0;

	virtual void PushShaderConstants(GraphicsShaderDataSet *set, int32_t constantIndex, int32_t constantOffset, int32_t constantCount, void *data) = 0;
	virtual void BindRenderShaderResourceInstance(GraphicsShaderDataSet *set, GraphicsShaderResourceInstance *instance, int32_t viewIndex, int32_t offset) = 0;
	virtual void BindComputeShaderResourceInstance(GraphicsShaderDataSet *set, GraphicsShaderResourceInstance *instance, int32_t viewIndex, int32_t offset) = 0;

	virtual void DrawIndexed(int32_t vertexCount, int32_t instances, int32_t firstIndex, int32_t vertexOffset, int32_t firstInstance) = 0;
	virtual void Draw(int32_t vertexCount, int32_t instanceCount, int32_t firstVertex, int32_t firstInstance) = 0;
	virtual void DrawIndirect(GraphicsMemoryBuffer *buffer, int32_t offset, int32_t drawCount, int32_t stride) = 0;
	virtual void DrawIndirectIndexed(GraphicsMemoryBuffer *buffer, int32_t offset, int32_t drawCount, int32_t stride) = 0;
	virtual void DrawIndirectCount(GraphicsMemoryBuffer *buffer, int32_t offset, GraphicsMemoryBuffer *drawCountBuffer, int32_t drawCountBufferOffset, int32_t maxDrawCount, int32_t stride) = 0;
	virtual void DrawIndirectIndexedCount(GraphicsMemoryBuffer *buffer, int32_t offset, GraphicsMemoryBuffer *drawCountBuffer, int32_t drawCountBufferOffset, int32_t maxDrawCount, int32_t stride) = 0;
	virtual void DispatchCompute(int32_t x, int32_t y, int32_t z) = 0;
	virtual void DispatchIndirect(GraphicsMemoryBuffer *buffer, int32_t offset) = 0;

	virtual void UpdatePipelineDynamicViewport(glm::vec4 dimensions) = 0;
	virtual void UpdatePipelineDynamicScissorBox(glm::vec4 dimensions) = 0;
	virtual void UpdatePipelineDynamicLineWidth(float lineWidth) = 0;
	virtual void UpdatePipelineDynamicStencilFrontFaceCompareMask(uint32_t mask) = 0;
	virtual void UpdatePipelineDynamicStencilFrontFaceWriteMask(uint32_t mask) = 0;
	virtual void UpdatePipelineDynamicStencilFrontFaceReference(uint32_t mask) = 0;
	virtual void UpdatePipelineDynamicStencilBackFaceCompareMask(uint32_t mask) = 0;
	virtual void UpdatePipelineDynamicStencilBackFaceWriteMask(uint32_t mask) = 0;
	virtual void UpdatePipelineDynamicStencilBackFaceReference(uint32_t mask) = 0;
	virtual void UpdatePipelineDynamicBlendConstant(glm::vec4 constants) = 0;

	virtual void WaitOnFence(GraphicsSyncObject *sync) = 0;
	virtual void SignalFence(GraphicsSyncObject *sync) = 0;
	virtual void ResetFence(GraphicsSyncObject *sync) = 0;

	virtual void SynchronizeMemory(MemoryBarrierBit from, MemoryBarrierBit to, bool byRegion) = 0;

	virtual void ClearColor(GraphicsImageObject *image, glm::vec4 color, int32_t level, int32_t layer) = 0;
	virtual void ClearDepthStencil(GraphicsImageObject *image, float depth, char stencil) = 0;

	virtual void ClearAttachmentsColor(int32_t index, glm::vec4 color) = 0;
	virtual void ClearAttachmentsDepthStencil(int32_t index, float depth, char stencil) = 0;

	virtual void UpdateBufferData(GraphicsMemoryBuffer *buffer, int32_t offset, int32_t size, void *data) = 0;

	virtual void CopyBufferToImageWithConversion(GraphicsMemoryBuffer *srcBuffer, VectorDataFormat srcFormat, GraphicsImageObject *destImage, std::vector<GraphicsBufferImageCopyRegion> regions) = 0;
	virtual void CopyImageToBuffer(GraphicsImageObject *srcImage, GraphicsMemoryBuffer *destBuffer, std::vector<GraphicsBufferImageCopyRegion> regions) = 0;

	virtual void CopyImageToImage(GraphicsImageObject *srcImage, GraphicsImageObject *destImage, glm::ivec3 srcOffset, glm::ivec3 destOffset, glm::ivec3 size,
		int32_t srcLevel, int32_t destLevel, int32_t layers, bool color, bool depth, bool stencil) = 0;
	virtual void CopyBufferToBuffer(GraphicsMemoryBuffer *src, GraphicsMemoryBuffer *dest, uint64_t srcOffset, uint64_t destOffset, int32_t size) = 0;

	virtual void BeginRecording() = 0;
	virtual void StopRecording() = 0;

	virtual void ExecuteSubCommandBuffer(GraphicsCommandBuffer *commandBuffer) = 0;
};

class GraphicsContext
{
public:
	virtual ~GraphicsContext() {}
	virtual std::vector<GraphicsCommandBuffer *> CreateGraphicsCommandBuffers(int32_t count, bool graphics, bool compute, bool transfer) = 0;
	virtual GraphicsCommandBuffer *GetTransferBufferFromPool() = 0;
	virtual GraphicsCommandBuffer *GetGraphicsBufferFromPool() = 0;
	virtual GraphicsCommandBuffer *GetComputeBufferFromPool() = 0;
	virtual GraphicsRenderPipeline *CreateGraphicsPipeline(GraphicsRenderPipelineState& state) = 0;
	virtual GraphicsComputePipeline *CreateComputePipeline(GraphicsComputePipelineState& state) = 0;
	virtual GraphicsMemoryBuffer *CreateBuffer(uint64_t byteSize, BufferUsageBit usage, GraphicsMemoryTypeBit mem) = 0;
	virtual GraphicsImageObject *CreateImage(ImageType type, VectorDataFormat format, glm::ivec3 size, int32_t miplevels, ImageUsageBit usage) = 0;
	virtual GraphicsRenderTarget *CreateRenderTarget(std::vector<GraphicsImageView *>&& attachments, GraphicsImageView *depthStencil, GraphicsRenderPass *renderPass, int32_t width, int32_t height, int32_t layers) = 0;
	virtual GraphicsShaderDataSet *CreateShaderDataSet(std::vector<GraphicsShaderResourceViewData>&& resourceViews, std::vector<GraphicsShaderConstantData>&& constantData) = 0;
	virtual GraphicsShaderResourceInstance *CreateShaderResourceInstance(GraphicsShaderResourceViewData& data) = 0;
	virtual GraphicsSampler *CreateSampler(GraphicsSamplerState& state) = 0;
	virtual GraphicsShader *CreateShader(GraphicsSpecificShaderCode *code) = 0;
	virtual GraphicsRenderPass *CreateRenderPass(GraphicsRenderPassState& state) = 0;
	virtual std::vector<GraphicsQuery *> CreateQueries(int32_t count, GraphicsQueryType type) = 0;
	virtual GraphicsSyncObject *CreateSync(bool gpuQueueSync) = 0;
	virtual GraphicsImageView *GetScreenImageView() = 0;
	virtual glm::ivec2 GetScreenSize() = 0;
	virtual GraphicsSpecificStructure& GetSpecificStructure() = 0;
	virtual bool IsOpenGLTextureFlipYConvention() = 0;
	virtual bool IsOpenGLNDCConvention() = 0;
	virtual GraphicsIdentity GetIdentity() = 0;
	virtual int32_t ConvertCubemapFaceToLayer(CubemapFace face) = 0;
	virtual void UpdateShaderImageSamplerResourceInstance(GraphicsShaderResourceInstance *instance, std::vector<GraphicsImageView *>&& imageViews, std::vector<GraphicsSampler *>&& samplers) = 0;
	virtual void UpdateShaderImageInputAttachmentInstance(GraphicsShaderResourceInstance *instance, std::vector<GraphicsImageView *>&& imageViews) = 0;
	virtual void UpdateShaderImageLoadStoreResourceInstance(GraphicsShaderResourceInstance *instance, std::vector<GraphicsImageView *>&& imageViews) = 0;
	virtual void UpdateShaderBufferResourceInstance(GraphicsShaderResourceInstance *instance, GraphicsMemoryBuffer *buffer, int32_t offset, int32_t range) = 0;
	virtual void SubmitCommands(GraphicsCommandBuffer *commands, GraphicsQueueType queue) = 0;
	virtual void Present() = 0;
	virtual void SyncWithCommandSubmissionThread() = 0;
};