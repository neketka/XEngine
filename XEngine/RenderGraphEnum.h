#pragma once

enum class RenderFormat 
{
	RGBA8Unorm, RGB8Unorm, R8G8Unorm, R8Unorm, RGBA32F, R32F, D24UnormS8Ui, D32F
};

enum class ImageType
{
	Image1D, Image2D, Image3D, ImageCubemap
};

enum class RenderPrimitiveType
{
	Triangles, Points, Lines, Patches
};

enum class ComparisonMode
{
	Never, Less, Equal, LessOrEqual, Greater, NotEqual, GreaterOrEqual, Always
};

enum class StencilOperation
{
	Keep, Zero, Replace, IncrementAndClamp, DecrementAndClamp, Invert, IncrementAndWrap, DecrementAndWrap
};

enum class BlendLogicOperation
{
	Clear, And, AndReverse, Copy, AndInverted, NoOp, Xor, Or, Nor, Equivalent, Invert, OrReverse, CopyInverted, OrInverted, Nand, Set
}; 

enum class BlendFactor
{
	Zero, One, SrcColor, OneMinusSrcColor, DestColor, OneMinusDestColor, SrcAlpha, OneMinusSrcAlpha, DestAlpha, OneMinusDestAlpha,
	ConstantColor, OneMinusConstantColor, ConstantAlpha, SrcAlphaSaturate, Src1Color, OneMinusSrc1Color, Src1Alpha, OneMinusSrc1Alpha
};

enum class BlendingOperation
{
	Add, Subtract, ReverseSubtract, Min, Max
};

enum class ShaderStageBit
{
	Vertex = 1, Geometry = 2, Fragment = 4, TessControl = 8, TessEval = 16, Compute = 32
};

enum class SamplerFilteringMode
{
	Nearest, Linear
};

enum class SamplerWrapMode
{
	Repeat, MirroredRepeat, ClampToEdge, ClampToBorder, MirrorClampToEdge
};

enum class PrimitiveType
{
	Triangles, Lines, LineStrip, Points
};

enum class PolygonMode
{
	Fill, Lines, Points
};

enum class ImageUsageBit : int
{
	Sampling = 1, Storage = 2, ColorAttachment = 4, DepthStencilAttachment = 8, InputAttachment = 16, TransferSource = 32, TransferDest = 64
};

enum class BufferUsageBit : int
{
	TransferSource = 1, TransferDest = 2, ConstantBuffer = 4, StorageBuffer = 8, IndexBuffer = 16, VertexBuffer = 32, IndirectCommandBuffer = 64
};

enum class ShaderViewType
{
	StorageImage, InputImage, ConstantBuffer, StorageBuffer, Sampler, SamplingImage, DynamicOffsetConstantBuffer, DynamicOffsetStorageBuffer
};

enum class ExecutionDependencyBits : int
{
	IndirectCommands = 1, IndexBuffer = 2, VertexBuffer = 4, ConstantBuffer = 8, InputImage = 16,
	ShaderAccess = 32, ColorAttachment = 64, DepthStencilAttachment = 128, Transfer = 256,
	HostAccess = 512, MemoryAccess = 1024
};

enum class QueryType
{
	Occlusion, Pipeline, Timestamp
};

enum class PipelineStatisticBits : int
{
	InputAssemblyVertices=1, InputAssemblyPrimitives=2, VertexShaderInvocations=4, GeometryShaderInvocations=8, 
	GeometryShaderPrimitives=16, ClippingInvocations=32, ClippingPrimitives=64, FragmentShaderInvocations=128,
	TessControlPatches=256, TessEvalInvocations=512, ComputeShaderInvocations=1024
};

enum class RenderResourceType
{
	NullType, GraphicsExecutionState, ComputeExecutionState, Buffer, Image, QuerySet, ShaderViewCollection, RenderPass,
	ShaderViewCollectionLayout, Shader, Sampler, ExternalResource
};