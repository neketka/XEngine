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
	virtual bool Wait(unsigned long long nanoSecTimeout) = 0;
	virtual bool GetCurrentStatus() = 0;
};

class GraphicsImageObject
{
public:
	virtual ~GraphicsImageObject() = 0;
	virtual GraphicsImageView *CreateView(ImageType type, VectorDataFormat format, int baseLevel, int levels, int baseLayer, int layers,
		ImageSwizzleComponent redSwizzle, ImageSwizzleComponent greenSizzle, ImageSwizzleComponent blueSwizzle,
		ImageSwizzleComponent alphaSwizzle) = 0;
};

class GraphicsShaderConstantData
{
public:
	ShaderStageBit Stages;
	int Offset;
	int Size;
	bool Float;
	bool Matrix;
	int VectorLength;
};

class GraphicsShaderResourceViewData
{
public:
	ShaderStageBit Stages;
	int Binding;
	ShaderResourceType Type;
	int Count;
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
	virtual void MapBuffer(bool coherent) = 0;
	virtual void MapBuffer(int offset, int length, bool coherent) = 0;
	virtual void UnmapBuffer() = 0;
	virtual void *GetMappedPointer() = 0;
	virtual void FlushMapped(int offset, int size) = 0;
	virtual void InvalidateMapped(int offset, int size) = 0;
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
	std::vector<int> InputAttachments;
	std::vector<int> ColorAttachments;
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
	int BufferBinding;
	int Stride;
	bool Instanced;
};

class GraphicsRenderVertexAttributeData
{
public:
	int ShaderAttributeLocation;
	int BufferBinding;
	VectorDataFormat Format;
	int InElementOffset;
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
	int PatchControlPoints;
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
	unsigned int CompareMask;
	unsigned int WriteMask;
	unsigned int Reference;
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
	int Level;
	int BufferOffset; 
	int BufferRowLength; 
	int BufferImageHeight;
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
	virtual void WriteQueryToBuffer(GraphicsQuery *query, int bufferOffset) = 0;

	virtual void BindVertexBuffers(int firstBinding, std::vector<GraphicsMemoryBuffer *> buffers, std::vector<int> offsets) = 0;
	virtual void BindIndexBuffer(GraphicsMemoryBuffer *buffer, bool dataType16bit) = 0;

	virtual void PushShaderConstants(GraphicsShaderDataSet *set, int constantIndex, int constantOffset, int constantCount, void *data) = 0;
	virtual void BindRenderShaderResourceInstance(GraphicsShaderDataSet *set, GraphicsShaderResourceInstance *instance, int viewIndex, int offset) = 0;
	virtual void BindComputeShaderResourceInstance(GraphicsShaderDataSet *set, GraphicsShaderResourceInstance *instance, int viewIndex, int offset) = 0;

	virtual void DrawIndexed(int vertexCount, int instances, int firstIndex, int vertexOffset, int firstInstance) = 0;
	virtual void Draw(int vertexCount, int instanceCount, int firstVertex, int firstInstance) = 0;
	virtual void DrawIndirect(GraphicsMemoryBuffer *buffer, int offset, int drawCount, int stride) = 0;
	virtual void DrawIndirectIndexed(GraphicsMemoryBuffer *buffer, int offset, int drawCount, int stride) = 0;
	virtual void DrawIndirectCount(GraphicsMemoryBuffer *buffer, int offset, GraphicsMemoryBuffer *drawCountBuffer, int drawCountBufferOffset, int maxDrawCount, int stride) = 0;
	virtual void DrawIndirectIndexedCount(GraphicsMemoryBuffer *buffer, int offset, GraphicsMemoryBuffer *drawCountBuffer, int drawCountBufferOffset, int maxDrawCount, int stride) = 0;
	virtual void DispatchCompute(int x, int y, int z) = 0;
	virtual void DispatchIndirect(GraphicsMemoryBuffer *buffer, int offset) = 0;

	virtual void UpdatePipelineDynamicViewport(glm::vec4 dimensions) = 0;
	virtual void UpdatePipelineDynamicScissorBox(glm::vec4 dimensions) = 0;
	virtual void UpdatePipelineDynamicLineWidth(float lineWidth) = 0;
	virtual void UpdatePipelineDynamicStencilFrontFaceCompareMask(unsigned int mask) = 0;
	virtual void UpdatePipelineDynamicStencilFrontFaceWriteMask(unsigned int mask) = 0;
	virtual void UpdatePipelineDynamicStencilFrontFaceReference(unsigned int mask) = 0;
	virtual void UpdatePipelineDynamicStencilBackFaceCompareMask(unsigned int mask) = 0;
	virtual void UpdatePipelineDynamicStencilBackFaceWriteMask(unsigned int mask) = 0;
	virtual void UpdatePipelineDynamicStencilBackFaceReference(unsigned int mask) = 0;
	virtual void UpdatePipelineDynamicBlendConstant(glm::vec4 constants) = 0;

	virtual void WaitOnFence(GraphicsSyncObject *sync) = 0;
	virtual void SignalFence(GraphicsSyncObject *sync) = 0;
	virtual void ResetFence(GraphicsSyncObject *sync) = 0;

	virtual void ClearColor(GraphicsImageObject *image, glm::vec4 color, int level, int layer) = 0;
	virtual void ClearDepthStencil(GraphicsImageObject *image, float depth, char stencil) = 0;

	virtual void ClearAttachmentsColor(int index, glm::vec4 color) = 0;
	virtual void ClearAttachmentsDepthStencil(int index, float depth, char stencil) = 0;

	virtual void UpdateBufferData(GraphicsMemoryBuffer *buffer, int offset, int size, void *data) = 0;

	virtual void CopyBufferToImage(GraphicsMemoryBuffer *srcBuffer, GraphicsImageObject *destImage, std::vector<GraphicsBufferImageCopyRegion> regions) = 0;
	virtual void CopyImageToBuffer(GraphicsImageObject *srcImage, GraphicsMemoryBuffer *destBuffer, std::vector<GraphicsBufferImageCopyRegion> regions) = 0;

	virtual void CopyImageToImage(GraphicsImageObject *srcImage, GraphicsImageObject *destImage, glm::ivec3 srcOffset, glm::ivec3 destOffset, glm::ivec3 size,
		int srcLevel, int destLevel, int layers, bool color, bool depth, bool stencil) = 0;
	virtual void CopyBufferToBuffer(GraphicsMemoryBuffer *src, GraphicsMemoryBuffer *dest, int srcOffset, int destOffset, int size) = 0;

	virtual void BeginRecording() = 0;
	virtual void StopRecording() = 0;

	virtual void ExecuteSubCommandBuffer(GraphicsCommandBuffer *commandBuffer) = 0;
};

class GraphicsContext
{
public:
	virtual ~GraphicsContext() {}
	virtual std::vector<GraphicsCommandBuffer *> CreateGraphicsCommandBuffers(int count, bool subBuffer, bool graphics, bool compute) = 0;
	virtual GraphicsRenderPipeline *CreateGraphicsPipeline(GraphicsRenderPipelineState& state) = 0;
	virtual GraphicsComputePipeline *CreateComputePipeline(GraphicsComputePipelineState& state) = 0;
	virtual GraphicsMemoryBuffer *CreateBuffer(int byteSize, BufferUsageBit usage, GraphicsMemoryTypeBit mem) = 0;
	virtual GraphicsImageObject *CreateImage(ImageType type, VectorDataFormat format, glm::ivec3 size, int miplevels, ImageUsageBit usage) = 0;
	virtual GraphicsRenderTarget *CreateRenderTarget(std::vector<GraphicsImageView *>&& attachments, GraphicsImageView *depthStencil, GraphicsRenderPass *renderPass, int width, int height, int layers) = 0;
	virtual GraphicsShaderDataSet *CreateShaderDataSet(std::vector<GraphicsShaderResourceViewData>&& resourceViews, std::vector<GraphicsShaderConstantData>&& constantData) = 0;
	virtual GraphicsShaderResourceInstance *CreateShaderResourceInstance(GraphicsShaderResourceViewData& data) = 0;
	virtual GraphicsSampler *CreateSampler(GraphicsSamplerState& state) = 0;
	virtual GraphicsShader *CreateShader(GraphicsSpecificShaderCode *code) = 0;
	virtual GraphicsRenderPass *CreateRenderPass(GraphicsRenderPassState& state) = 0;
	virtual std::vector<GraphicsQuery *> CreateQueries(int count, GraphicsQueryType type) = 0;
	virtual GraphicsSyncObject *CreateSync(bool gpuQueueSync) = 0;
	virtual GraphicsImageView *GetScreenImageView() = 0;
	virtual glm::ivec2 GetScreenSize() = 0;
	virtual GraphicsSpecificStructure& GetSpecificStructure() = 0;
	virtual void UpdateShaderImageSamplerResourceInstance(GraphicsShaderResourceInstance *instance, std::vector<GraphicsImageView *>&& imageViews, std::vector<GraphicsSampler *>&& samplers) = 0;
	virtual void UpdateShaderImageInputAttachmentInstance(GraphicsShaderResourceInstance *instance, std::vector<GraphicsImageView *>&& imageViews) = 0;
	virtual void UpdateShaderImageLoadStoreResourceInstance(GraphicsShaderResourceInstance *instance, std::vector<GraphicsImageView *>&& imageViews) = 0;
	virtual void UpdateShaderBufferResourceInstance(GraphicsShaderResourceInstance *instance, GraphicsMemoryBuffer *buffer, int offset, int range) = 0;
	virtual void SubmitCommands(GraphicsCommandBuffer *commands, GraphicsQueueType queue) = 0;
	virtual void Present() = 0;
	virtual void SyncWithCommandSubmissionThread() = 0;
};