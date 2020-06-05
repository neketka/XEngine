#pragma once

enum class VectorDataFormat
{
	R8Unorm, R8Snorm, R16Unorm, R16Snorm, R16G16Unorm, R16G16Snorm, R3G3B2Unorm, R4G4B4Unorm, R5G5B5Unorm, R8G8B8Unorm,
	R8G8B8Snorm, R10G10B10Unorm, R12G12B12Unorm, R16G16B16Snorm, R2G2B2A2Unorm, R4G4B4A4Unorm, R5G5B5A1Unorm, R8G8B8A8Unorm,
	R8G8B8A8Snorm, R10G10B10A2Unorm, R10G10B10A2Uint, R12G12B12A12Unorm, R16B16G16A16Unorm, SR8G8B8Unorm, SR8G8B8A8Unorm,
	R16Float, R16G16Float, R16G16B16Float, R16G16B16A16Float, R32Float, R32G32Float, R32G32B32Float, R32G32B32A32Float,
	R11G11B10Float, R8Int, R8Uint, R16Int, R16Uint, R32Int, R32Uint, R8G8Int, R8G8Uint, R16G16Int, R16G16Uint, R32G32Int, R32G32Uint,
	R8G8B8Int, R8G8B8Uint, R16G16B16Int, R16G16B16Uint, R32G32B32Int, R32G32B32Uint, R8G8B8A8Int, R8G8B8A8Uint, 
	R16G16B16A16Int, R16G16B16A16Uint, R32G32B32A32Int, R32G32B32A32Uint, D16Unorm, D24Unorm, D32Float, D24UnormS8Uint, D32FloatS8Uint, 
	S8Uint
};

enum class ImageType
{
	Image1D, Image1DArray, Image2D, Image2DArray, Image3D, ImageCubemap, ImageCubemapArray
};

enum class GraphicsQueueType
{
	Graphics, Compute, Transfer
};

enum class GraphicsPrimitiveType
{
	Triangles, Points, Lines, Patches
};

enum class GraphicsRenderPassLoadStore
{
	Discard, Preserve, Clear
};

enum class PolygonMode
{
	Fill, Line, Point
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

enum class BufferUsageBit
{
	TransferSource=1, TransferDest=2, UniformBuffer=4, StorageBuffer=8, IndexBuffer=16, VertexBuffer=32, IndirectCommands=64
};

inline BufferUsageBit operator|(BufferUsageBit a, BufferUsageBit b)
{
	return static_cast<BufferUsageBit>(static_cast<int>(a) | static_cast<int>(b));
}

inline bool operator&(BufferUsageBit a, BufferUsageBit b)
{
	return static_cast<bool>(static_cast<int>(a) & static_cast<int>(b));
}

enum class ImageUsageBit
{
	TransferSource=1, TransferDest=2, Sampling=4, LoadStore=8, Color, DepthStencil=16, ShaderInput=32
};

inline ImageUsageBit operator|(ImageUsageBit a, ImageUsageBit b)
{
	return static_cast<ImageUsageBit>(static_cast<int>(a) | static_cast<int>(b));
}

inline bool operator&(ImageUsageBit a, ImageUsageBit b)
{
	return static_cast<bool>(static_cast<int>(a) & static_cast<int>(b));
}

enum class ImageSwizzleComponent
{
	Red, Green, Blue, Alpha, Zero, One
};

enum class ShaderStageBit
{
	Vertex=1, Geometry=2, Fragment=4, TessControl=8, TessEval=16, Compute=32
};

inline ShaderStageBit operator|(ShaderStageBit a, ShaderStageBit b)
{
	return static_cast<ShaderStageBit>(static_cast<int>(a) | static_cast<int>(b));
}

inline bool operator&(ShaderStageBit a, ShaderStageBit b)
{
	return static_cast<bool>(static_cast<int>(a) & static_cast<int>(b));
}

enum class ShaderResourceType
{
	ImageAndSampler, UniformBuffer, StorageBuffer, ImageLoadStore, UniformBufferDynamic, StorageBufferDynamic, InputAttachment
};

enum class SamplerFilteringMode
{
	Nearest, Linear
};

enum class SamplerWrapMode
{
	Repeat, MirroredRepeat, ClampToEdge, ClampToBorder, MirrorClampToEdge
};

enum class GraphicsQueryType
{
	AnySamplesPassed, PrimitivesGenerated, Timestamp
};

enum class GraphicsMemoryTypeBit
{
	DynamicAccess, HostVisible, Coherent, DeviceResident
};

inline GraphicsMemoryTypeBit operator|(GraphicsMemoryTypeBit a, GraphicsMemoryTypeBit b)
{
	return static_cast<GraphicsMemoryTypeBit>(static_cast<int>(a) | static_cast<int>(b));
}

inline bool operator&(GraphicsMemoryTypeBit a, GraphicsMemoryTypeBit b)
{
	return static_cast<bool>(static_cast<int>(a) & static_cast<int>(b));
}