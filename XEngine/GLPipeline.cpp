#include "pch.h"
#include "GLPipeline.h"
#include "GLShaderData.h"
#include "GLImage.h"
#include "GLContext.h"

GLPipeline::GLPipeline(GraphicsContext *context, GraphicsRenderPipelineState& state) : m_state(state), m_context(context)
{
}

GLPipeline::~GLPipeline()
{
	GLuint vao = m_vao;
	GLuint pipeline = m_pipeline;
	dynamic_cast<GLContext *>(m_context)->DeleteInitable([vao, pipeline]() {
		glDeleteVertexArrays(1, &vao);
		glDeleteProgramPipelines(1, &pipeline);
	});
}

std::map<ComparisonMode, GLenum> compMode = 
{
	{ ComparisonMode::Never, GL_NEVER }, { ComparisonMode::Less, GL_LESS }, { ComparisonMode::Equal, GL_EQUAL },
	{ ComparisonMode::LessOrEqual, GL_LEQUAL }, { ComparisonMode::Greater, GL_GREATER }, { ComparisonMode::NotEqual, GL_NOTEQUAL },
	{ ComparisonMode::GreaterOrEqual, GL_GEQUAL }, { ComparisonMode::Always, GL_ALWAYS }
};

std::map<StencilOperation, GLenum> stencilOp =
{
	{ StencilOperation::Keep, GL_KEEP }, { StencilOperation::Zero, GL_ZERO }, { StencilOperation::Replace, GL_REPLACE },
	{ StencilOperation::IncrementAndClamp, GL_INCR }, { StencilOperation::DecrementAndClamp, GL_DECR }, { StencilOperation::Invert, GL_INVERT },
	{ StencilOperation::IncrementAndWrap, GL_INCR_WRAP }, { StencilOperation::DecrementAndWrap, GL_DECR_WRAP }
}; 

std::map<BlendLogicOperation, GLenum> blendLogicOp =
{
	{ BlendLogicOperation::Clear, GL_CLEAR }, { BlendLogicOperation::And, GL_AND }, { BlendLogicOperation::AndReverse, GL_AND_REVERSE },
	{ BlendLogicOperation::Copy, GL_COPY }, { BlendLogicOperation::AndInverted, GL_AND_INVERTED }, { BlendLogicOperation::NoOp, GL_NOOP },
	{ BlendLogicOperation::Xor, GL_XOR }, { BlendLogicOperation::Or, GL_OR }, { BlendLogicOperation::Nor, GL_NOR }, { BlendLogicOperation::Equivalent, GL_EQUIV },
	{ BlendLogicOperation::Invert, GL_INVERT }, { BlendLogicOperation::OrReverse, GL_OR_REVERSE }, { BlendLogicOperation::CopyInverted, GL_COPY_INVERTED },
	{ BlendLogicOperation::OrInverted, GL_OR_INVERTED }, { BlendLogicOperation::Nand, GL_NAND }, { BlendLogicOperation::Set, GL_SET}
};

std::map<BlendFactor, GLenum> blendFactor =
{
	{ BlendFactor::Zero, GL_ZERO }, { BlendFactor::One, GL_ONE }, { BlendFactor::SrcColor, GL_SRC_COLOR }, { BlendFactor::OneMinusSrcColor, GL_ONE_MINUS_SRC_COLOR },
	{ BlendFactor::DestColor, GL_DST_COLOR }, { BlendFactor::OneMinusDestColor, GL_ONE_MINUS_DST_COLOR }, { BlendFactor::SrcAlpha, GL_SRC_ALPHA  },
	{ BlendFactor::OneMinusSrcAlpha, GL_ONE_MINUS_SRC_ALPHA }, { BlendFactor::DestAlpha, GL_DST_ALPHA }, { BlendFactor::OneMinusDestAlpha, GL_ONE_MINUS_DST_ALPHA },
	{ BlendFactor::ConstantColor, GL_CONSTANT_COLOR }, { BlendFactor::OneMinusConstantColor, GL_ONE_MINUS_CONSTANT_COLOR }, 
	{ BlendFactor::ConstantAlpha, GL_CONSTANT_ALPHA }, { BlendFactor::SrcAlphaSaturate, GL_SRC_ALPHA_SATURATE }, { BlendFactor::Src1Color, GL_SRC1_COLOR },
	{ BlendFactor::OneMinusSrc1Color, GL_ONE_MINUS_SRC1_COLOR }, { BlendFactor::Src1Alpha, GL_SRC1_COLOR }, { BlendFactor::OneMinusSrc1Alpha, GL_ONE_MINUS_SRC1_ALPHA }
};

std::map<BlendingOperation, GLenum> blendOp =
{
	{ BlendingOperation::Add, GL_FUNC_ADD }, { BlendingOperation::Subtract, GL_FUNC_SUBTRACT }, { BlendingOperation::ReverseSubtract, GL_FUNC_REVERSE_SUBTRACT },
	{ BlendingOperation::Min, GL_MIN }, { BlendingOperation::Max, GL_MAX }
};

void GLPipeline::BindState()
{
	glUseProgram(0);
	glBindProgramPipeline(m_pipeline);
	glBindVertexArray(m_vao);

	// Viewport

	glViewport(m_state.ViewportState.ViewportBounds.x, m_state.ViewportState.ViewportBounds.y, m_state.ViewportState.ViewportBounds.z, m_state.ViewportState.ViewportBounds.w);
	glEnable(GL_SCISSOR_TEST);
	glScissor(m_state.ViewportState.ScissorBounds.x, m_state.ViewportState.ScissorBounds.y, m_state.ViewportState.ScissorBounds.z, m_state.ViewportState.ScissorBounds.w);

	if (m_state.RasterState.EnableState)
	{
		if (m_state.RasterState.CullFrontFace && m_state.RasterState.CullBackFace)
		{
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT_AND_BACK);
		}
		else if (m_state.RasterState.CullFrontFace)
		{
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT);
		}
		else if (m_state.RasterState.CullBackFace)
		{
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
		}
		else
			glDisable(GL_CULL_FACE);

		if (m_state.RasterState.DoDepthClamp)
			glEnable(GL_DEPTH_CLAMP);
		else
			glDisable(GL_DEPTH_CLAMP);

		glFrontFace(m_state.RasterState.ClockwiseFrontFace ? GL_CW : GL_CCW);
		glPolygonMode(GL_FRONT_AND_BACK, m_state.RasterState.DrawingMode == PolygonMode::Fill ? GL_FILL :(m_state.RasterState.DrawingMode == PolygonMode::Line 
			? GL_LINE : (m_state.RasterState.DrawingMode == PolygonMode::Point ? GL_POINT : GL_FILL)));

		glLineWidth(m_state.RasterState.LineWidth);
		
		if (m_state.RasterState.RasterizerDiscard)
			glEnable(GL_RASTERIZER_DISCARD);
		else
			glDisable(GL_RASTERIZER_DISCARD);
	}

	if (m_state.RenderTessellationState.EnableState)
	{
		glPatchParameteri(GL_PATCH_VERTICES, m_state.RenderTessellationState.PatchControlPoints);
	}

	if (m_state.DepthStencilState.EnableState)
	{
		if (m_state.DepthStencilState.DepthTest)
			glEnable(GL_DEPTH_TEST);
		else
			glDisable(GL_DEPTH_TEST);

		if (m_state.DepthStencilState.StencilTest)
			glEnable(GL_STENCIL_TEST);
		else
			glDisable(GL_STENCIL_TEST);
		
		glDepthMask(m_state.DepthStencilState.DepthWrite);
		glDepthFunc(compMode[m_state.DepthStencilState.DepthCompareOp]);

		glStencilOpSeparate(GL_FRONT, stencilOp[m_state.DepthStencilState.StencilFront.Fail], stencilOp[m_state.DepthStencilState.StencilFront.DepthFail], 
			stencilOp[m_state.DepthStencilState.StencilFront.Pass]);
		glStencilOpSeparate(GL_BACK, stencilOp[m_state.DepthStencilState.StencilBack.Fail], stencilOp[m_state.DepthStencilState.StencilBack.DepthFail],
			stencilOp[m_state.DepthStencilState.StencilBack.Pass]);

		glStencilFuncSeparate(GL_FRONT, compMode[m_state.DepthStencilState.StencilFront.CompareOperation],
			m_state.DepthStencilState.StencilFront.Reference, m_state.DepthStencilState.StencilFront.CompareMask);
		glStencilFuncSeparate(GL_BACK, compMode[m_state.DepthStencilState.StencilBack.CompareOperation],
			m_state.DepthStencilState.StencilBack.Reference, m_state.DepthStencilState.StencilBack.CompareMask);

		glStencilMaskSeparate(GL_FRONT, m_state.DepthStencilState.StencilFront.WriteMask);
		glStencilMaskSeparate(GL_BACK, m_state.DepthStencilState.StencilBack.WriteMask);
	}

	// Blending
	glBlendColor(m_state.BlendingState.BlendConstants.r, m_state.BlendingState.BlendConstants.g, 
		m_state.BlendingState.BlendConstants.b, m_state.BlendingState.BlendConstants.a);
	if (m_state.BlendingState.DoLogicOperations)
	{
		glEnable(GL_COLOR_LOGIC_OP);
		glLogicOp(blendLogicOp[m_state.BlendingState.LogicOperation]);
	}
	else
	{
		glDisable(GL_COLOR_LOGIC_OP);
		int i = 0;
		for (GraphicsRenderBlendingAttachmentState& state : m_state.BlendingState.BufferBlending)
		{
			if (state.Blending)
			{
				glEnablei(GL_BLEND, i);
				glBlendEquationSeparatei(i, blendOp[state.ColorBlendOperation], blendOp[state.AlphaBlendOperation]);
				glBlendFuncSeparatei(i, blendFactor[state.SrcColorBlendFactor], blendFactor[state.DestColorBlendFactor],
					blendFactor[state.SrcAlphaBlendFactor], blendFactor[state.DestAlphaBlendFactor]);
				glColorMaski(i, state.ColorWriteMask.r, state.ColorWriteMask.g, state.ColorWriteMask.b, state.ColorWriteMask.a);
			}
			else
				glDisablei(GL_BLEND, i);
			++i;
		}
	}
}

void GLPipeline::InitInternal()
{
	glGenProgramPipelines(1, &m_pipeline);
	glCreateVertexArrays(1, &m_vao);
	for (GraphicsShaderState& state : m_state.Shaders)
	{
		GLShader *shader = dynamic_cast<GLShader *>(state.Shader);
		GLuint sid = shader->GetSpecialization(dynamic_cast<GLShaderSpecialization *>(state.Specialization));
		glUseProgramStages(m_pipeline, ConvertFromShaderStageBit(shader->GetShaderCode().GetStage()), sid);
		m_stageToProgram[shader->GetShaderCode().GetStage()] = sid;
	}
	for (GraphicsRenderVertexAttributeData& data : m_state.VertexInputState.AttributeData)
	{
		VectorFormatDesc desc = GetFormatDesc(data.Format);
		int dataSize = (desc.RBits > 0 ? 1 : 0) + (desc.GBits > 0 ? 1 : 0) + (desc.BBits > 0 ? 1 : 0) + (desc.ABits > 0 ? 1 : 0);
		int maxBit = glm::max<int>(desc.RBits, glm::max<int>(desc.GBits, glm::max<int>(desc.BBits, desc.ABits)));
		glEnableVertexArrayAttrib(m_vao, data.ShaderAttributeLocation);
		glVertexArrayAttribBinding(m_vao, data.ShaderAttributeLocation, data.BufferBinding);
		if (desc.Integer)
		{
			GLenum type = maxBit == 8 ? (desc.Unsigned ? GL_UNSIGNED_BYTE : GL_BYTE) : (maxBit == 16 ? (desc.Unsigned ? GL_UNSIGNED_SHORT : GL_SHORT) : 
				(maxBit == 32 ? (desc.Unsigned ? GL_UNSIGNED_INT : GL_INT) : 0));
			glVertexArrayAttribIFormat(m_vao, data.ShaderAttributeLocation, dataSize, type, data.InElementOffset);
		}
		else
			glVertexArrayAttribFormat(m_vao, data.ShaderAttributeLocation, dataSize, desc.Long ? GL_DOUBLE : GL_FLOAT, desc.Normalized, data.InElementOffset);
	}
	for (GraphicsRenderVertexBufferInputData& data : m_state.VertexInputState.BufferData)
	{
		glVertexArrayBindingDivisor(m_vao, data.BufferBinding, data.Instanced ? 1 : 0);
		m_bindingStrides[data.BufferBinding] = data.Stride;
	}
}

GLComputePipeline::GLComputePipeline(GraphicsContext *context, GraphicsComputePipelineState& state) : m_state(state), m_context(context)
{
}

GLComputePipeline::~GLComputePipeline()
{
	GLuint pipeline = m_pipeline;
	dynamic_cast<GLContext *>(m_context)->DeleteInitable([pipeline]() {
		glDeleteProgramPipelines(1, &pipeline);
	});
}

void GLComputePipeline::BindState()
{
	glUseProgram(0);
	glBindProgramPipeline(m_pipeline);
}

void GLComputePipeline::InitInternal()
{
	glGenProgramPipelines(1, &m_pipeline);
	GLShader *shader = dynamic_cast<GLShader *>(m_state.ShaderState.Shader);
	GLuint sid = shader->GetSpecialization(dynamic_cast<GLShaderSpecialization *>(m_state.ShaderState.Specialization));
	glUseProgramStages(m_pipeline, ConvertFromShaderStageBit(shader->GetShaderCode().GetStage()), sid);
	m_program = sid;
}
