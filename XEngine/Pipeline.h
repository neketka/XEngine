#pragma once
#include <glm\glm.hpp>

#include "Shader.h"
#include "Buffer.h"
#include "VertexArray.h"
#include "Framebuffer.h"
#include "Quad.h"
#include "Sampler.h"

class DrawArraysIndirectCommand
{
public:
	GLuint Count;
	GLuint InstanceCount;
	GLuint First;
	GLuint BaseInstance;
};

class DrawElementsIndirectCommand
{
public:
	GLuint Count;
	GLuint InstanceCount;
	GLuint FirstIndex;
	GLuint BaseVertex;
	GLuint BaseInstance;
};

enum class StencilOperation
{
	Keep = GL_KEEP, Zero = GL_ZERO, Increment = GL_INCR, Decrement = GL_DECR, Invert = GL_INVERT,
	Replace = GL_REPLACE, IncrementWrap = GL_INCR_WRAP, DecrementWrap = GL_DECR_WRAP
};

enum class BlendEquation
{
	Add=GL_FUNC_ADD, Subtract=GL_FUNC_SUBTRACT, ReverseSubtract=GL_FUNC_REVERSE_SUBTRACT
};

enum class BlendFunction
{
	Zero = GL_ZERO, One = GL_ONE, SourceColor = GL_SRC_COLOR, OneMinusSourceColor = GL_ONE_MINUS_SRC_COLOR,
	DestinationColor = GL_DST_COLOR, OneMinusDestinationColor = GL_ONE_MINUS_DST_COLOR, SourceAlpha = GL_SRC_ALPHA,
	OneMinusSourceAlpha = GL_ONE_MINUS_SRC_ALPHA, DestinationAlpha = GL_DST_ALPHA, OneMinusDestinationAlpha = GL_ONE_MINUS_DST_ALPHA,
	ConstantColor = GL_CONSTANT_COLOR, OneMinusConstantColor = GL_ONE_MINUS_CONSTANT_COLOR, ConstantAlpha = GL_CONSTANT_ALPHA,
	OneMinusConstantAlpha = GL_ONE_MINUS_CONSTANT_ALPHA
};

class ColorMask
{
public:
	int Buffer;
	bool Red;
	bool Green;
	bool Blue;
	bool Alpha;
};

class BufferBlending
{
public:
	int Buffer;

	BlendFunction SourceColor;
	BlendFunction DestinationColor;
	BlendFunction SourceAlpha;
	BlendFunction DestinationAlpha;

	BlendEquation ColorEquation;
	BlendEquation AlphaEquation;
};

class VertexStage
{
public:
	VertexStage() {}
	
	VertexArray VertexArray;
	
	bool FrontFaceCulling = false;
	bool BackFaceCulling = false;
	bool DepthClamping = false;

	bool _keepAttributes = false;
};

class TessellationStage
{
public:
	TessellationStage() {}
	int PatchSize = 1;
};

class FragmentStage
{
public:
	FragmentStage() : Framebuffer(Framebuffer::GetDefault()), FrontStencilFunc(ComparisonFunc::Less), BackStencilFunc(ComparisonFunc::Less) {
	}
	Framebuffer Framebuffer;
	std::vector<int> DrawTargets;
	
	IQuad2D Viewport;
	IQuad2D ScissorRegion;
	bool ScissorTest = false;
	bool RasterizerDiscard = false;
	bool DepthTest = false;
	bool StencilTest = false;

	bool DepthMask = true;
	GLuint FrontStencilMask;
	GLuint BackStencilMask;
	std::vector<ColorMask> ColorMasks;

	ComparisonFunc FrontStencilFunc;
	ComparisonFunc BackStencilFunc;
	GLint StencilCompareReference;
	GLuint StencilCompareMask;

	StencilOperation FrontStencilFail;
	StencilOperation FrontDepthFail;
	StencilOperation FrontDepthPass;

	StencilOperation BackStencilFail;
	StencilOperation BackDepthFail;
	StencilOperation BackDepthPass;

	bool Blending = false;
	std::vector<BufferBlending> BufferBlends;

	bool _keepFBO = false;
	bool _keepTargets = false;
};

class TransformFeedbackStage
{
public:
	TransformFeedbackStage() {}
	std::vector<std::pair<int, TBuffer>> BufferBindings;

	bool _keepBindings = false;
};

class Pipeline
{
public:
	Shader Shader;
	VertexStage VertexStage;
	TessellationStage TessellationStage;
	TransformFeedbackStage TransformFeedbackStage;
	FragmentStage FragmentStage;
	
	void Optimize(Pipeline& previous);

	void BindPipeline();

private:
	bool m_keepShader;
};

