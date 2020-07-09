#include "pch.h"
#include "GLImage.h"
#include "GLContext.h"

std::vector<std::pair<VectorFormatDesc, GLenum>> internalFormatAsList =
{//              R/D/S, G/S, B, A, Int, Norm, Unsign, SRGB, Long, Depth, Stencil
	{ VectorFormatDesc(8, 0, 0, 0, false, true, true, false, false, false, false), GL_R8 },
	{ VectorFormatDesc(8, 0, 0, 0, false, true, false, false, false, false, false), GL_R8_SNORM },
	{ VectorFormatDesc(16, 0, 0, 0, false, true, true, false, false, false, false), GL_R16 },
	{ VectorFormatDesc(16, 0, 0, 0, false, true, false, false, false, false, false), GL_R16_SNORM },
	{ VectorFormatDesc(16, 16, 0, 0, false, true, true, false, false, false, false), GL_RG16 },
	{ VectorFormatDesc(16, 16, 0, 0, false, true, false, false, false, false, false), GL_RG16_SNORM },
	{ VectorFormatDesc(3, 3, 2, 0, false, true, true, false, false, false, false), GL_R3_G3_B2 },
	{ VectorFormatDesc(4, 4, 4, 0, false, true, true, false, false, false, false), GL_RGB4 },
	{ VectorFormatDesc(5, 5, 5, 0, false, true, true, false, false, false, false), GL_RGB5 },
	{ VectorFormatDesc(8, 8, 8, 0, false, true, true, false, false, false, false), GL_RGB8 },
	{ VectorFormatDesc(8, 8, 8, 0, false, true, false, false, false, false, false), GL_RGB8_SNORM },
	{ VectorFormatDesc(10, 10, 10, 0, false, true, true, false, false, false, false), GL_RGB10 },
	{ VectorFormatDesc(12, 12, 12, 0, false, true, true, false, false, false, false), GL_RGB12 },
	{ VectorFormatDesc(16, 16, 16, 0, false, true, false, false, false, false, false), GL_RGB16_SNORM },
	{ VectorFormatDesc(2, 2, 2, 2, false, true, true, false, false, false, false), GL_RGBA2 },
	{ VectorFormatDesc(4, 4, 4, 4, false, true, true, false, false, false, false), GL_RGBA4 },
	{ VectorFormatDesc(5, 5, 5, 1, false, true, true, false, false, false, false), GL_RGB5_A1 },
	{ VectorFormatDesc(8, 8, 8, 8, false, true, true, false, false, false, false), GL_RGBA8 },
	{ VectorFormatDesc(8, 8, 8, 8, false, true, false, false, false, false, false), GL_RGBA8_SNORM },
	{ VectorFormatDesc(10, 10, 10, 2, false, true, true, false, false, false, false), GL_RGB10_A2 },
	{ VectorFormatDesc(10, 10, 10, 2, true, true, true, false, false, false, false), GL_RGB10_A2UI },
	{ VectorFormatDesc(12, 12, 12, 12, false, true, true, false, false, false, false), GL_RGBA12 },
	{ VectorFormatDesc(16, 16, 16, 16, false, true, true, false, false, false, false), GL_RGBA16 },
	{ VectorFormatDesc(8, 8, 8, 0, false, true, true, true, false, false, false), GL_SRGB8 },
	{ VectorFormatDesc(8, 8, 8, 8, false, true, true, true, false, false, false), GL_SRGB8_ALPHA8 },
	{ VectorFormatDesc(16, 0, 0, 0, false, false, false, false, false, false, false), GL_R16F },
	{ VectorFormatDesc(16, 16, 0, 0, false, false, false, false, false, false, false), GL_RG16F },
	{ VectorFormatDesc(16, 16, 16, 0, false, false, false, false, false, false, false), GL_RGB16F },
	{ VectorFormatDesc(16, 16, 16, 16, false, false, false, false, false, false, false), GL_RGBA16F },
	{ VectorFormatDesc(32, 0, 0, 0, false, false, false, false, false, false, false), GL_R32F },
	{ VectorFormatDesc(32, 32, 0, 0, false, false, false, false, false, false, false), GL_RG32F },
	{ VectorFormatDesc(32, 32, 32, 0, false, false, false, false, false, false, false), GL_RGB32F },
	{ VectorFormatDesc(32, 32, 32, 32, false, false, false, false, false, false, false), GL_RGBA32F },
	{ VectorFormatDesc(11, 11, 10, 0, false, false, false, false, false, false, false), GL_R11F_G11F_B10F },
	{ VectorFormatDesc(8, 0, 0, 0, true, false, false, false, false, false, false), GL_R8I },
	{ VectorFormatDesc(8, 0, 0, 0, true, false, true, false, false, false, false), GL_R8UI },
	{ VectorFormatDesc(16, 0, 0, 0, true, false, false, false, false, false, false), GL_R16I },
	{ VectorFormatDesc(16, 0, 0, 0, true, false, true, false, false, false, false), GL_R16UI },
	{ VectorFormatDesc(32, 0, 0, 0, true, false, false, false, false, false, false), GL_R32I },
	{ VectorFormatDesc(32, 0, 0, 0, true, false, true, false, false, false, false), GL_R32UI },
	{ VectorFormatDesc(8, 8, 0, 0, true, false, false, false, false, false, false), GL_RG8I },
	{ VectorFormatDesc(8, 8, 0, 0, true, false, true, false, false, false, false), GL_RG8UI },
	{ VectorFormatDesc(16, 16, 0, 0, true, false, false, false, false, false, false), GL_RG16I },
	{ VectorFormatDesc(16, 16, 0, 0, true, false, true, false, false, false, false), GL_RG16UI },
	{ VectorFormatDesc(32, 32, 0, 0, true, false, false, false, false, false, false), GL_RG32I },
	{ VectorFormatDesc(32, 32, 0, 0, true, false, true, false, false, false, false), GL_RG32UI },
	{ VectorFormatDesc(8, 8, 8, 0, true, false, false, false, false, false, false), GL_RGB8I },
	{ VectorFormatDesc(8, 8, 8, 0, true, false, true, false, false, false, false), GL_RGB8UI },
	{ VectorFormatDesc(16, 16, 16, 0, true, false, false, false, false, false, false), GL_RGB16I },
	{ VectorFormatDesc(16, 16, 16, 0, true, false, true, false, false, false, false), GL_RGB16UI },
	{ VectorFormatDesc(32, 32, 32, 0, true, false, false, false, false, false, false), GL_RGB32I },
	{ VectorFormatDesc(32, 32, 32, 0, true, false, true, false, false, false, false), GL_RGB32UI },
	{ VectorFormatDesc(8, 8, 8, 8, true, false, false, false, false, false, false), GL_RGBA8I },
	{ VectorFormatDesc(8, 8, 8, 8, true, false, true, false, false, false, false), GL_RGBA8UI },
	{ VectorFormatDesc(16, 16, 16, 16, true, false, false, false, false, false, false), GL_RGBA16I },
	{ VectorFormatDesc(16, 16, 16, 16, true, false, true, false, false, false, false), GL_RGBA16UI },
	{ VectorFormatDesc(32, 32, 32, 32, true, false, false, false, false, false, false), GL_RGBA32I },
	{ VectorFormatDesc(32, 32, 32, 32, true, false, true, false, false, false, false), GL_RGBA32UI },
	{ VectorFormatDesc(16, 0, 0, 0, true, true, true, false, false, true, false), GL_DEPTH_COMPONENT16 },
	{ VectorFormatDesc(24, 0, 0, 0, true, true, true, false, false, true, false), GL_DEPTH_COMPONENT24 },
	{ VectorFormatDesc(32, 0, 0, 0, true, true, true, false, false, true, false), GL_DEPTH_COMPONENT32F },
	{ VectorFormatDesc(24, 8, 0, 0, true, true, true, false, false, true, true), GL_DEPTH24_STENCIL8 },
	{ VectorFormatDesc(32, 8, 0, 0, true, true, true, false, false, false, false), GL_DEPTH32F_STENCIL8 },
	{ VectorFormatDesc(8, 0, 0, 0, true, true, true, false, false, false, false), GL_STENCIL_INDEX8 },
};

std::unordered_map<ImageType, GLenum> typeToEnum = 
{
	{ ImageType::Image1D, GL_TEXTURE_1D },
	{ ImageType::Image1DArray, GL_TEXTURE_1D_ARRAY },
	{ ImageType::Image2D, GL_TEXTURE_2D },
	{ ImageType::Image2DArray, GL_TEXTURE_2D_ARRAY },
	{ ImageType::Image3D, GL_TEXTURE_3D},
	{ ImageType::ImageCubemap, GL_TEXTURE_CUBE_MAP },
	{ ImageType::ImageCubemapArray, GL_TEXTURE_CUBE_MAP_ARRAY },
};

std::unordered_map<ImageSwizzleComponent, GLint> swizzleToInt =
{
	{ ImageSwizzleComponent::Alpha, GL_ALPHA },
	{ ImageSwizzleComponent::Blue, GL_BLUE },
	{ ImageSwizzleComponent::Green, GL_GREEN },
	{ ImageSwizzleComponent::One, GL_ONE },
	{ ImageSwizzleComponent::Red, GL_RED },
	{ ImageSwizzleComponent::Zero, GL_ZERO }
};

std::unordered_map<ComparisonMode, GLenum> compMode =
{
	{ ComparisonMode::Never, GL_NEVER }, 
	{ ComparisonMode::Less, GL_LESS }, 
	{ ComparisonMode::Equal, GL_EQUAL },
	{ ComparisonMode::LessOrEqual, GL_LEQUAL }, 
	{ ComparisonMode::Greater, GL_GREATER }, 
	{ ComparisonMode::NotEqual, GL_NOTEQUAL },
	{ ComparisonMode::GreaterOrEqual, GL_GEQUAL }, 
	{ ComparisonMode::Always, GL_ALWAYS }
};

std::unordered_map<SamplerWrapMode, GLenum> wrapMode =
{
	{ SamplerWrapMode::ClampToBorder, GL_CLAMP_TO_BORDER },
	{ SamplerWrapMode::ClampToEdge, GL_CLAMP_TO_EDGE },
	{ SamplerWrapMode::MirrorClampToEdge, GL_MIRROR_CLAMP_TO_EDGE },
	{ SamplerWrapMode::MirroredRepeat, GL_MIRRORED_REPEAT },
	{ SamplerWrapMode::Repeat, GL_REPEAT }
};

VectorFormatDesc GetFormatDesc(VectorDataFormat format)
{
	return internalFormatAsList[static_cast<int>(format)].first;
}

GLenum GetInternalFormatFromFormat(VectorDataFormat format)
{
	return internalFormatAsList[static_cast<int>(format)].second;
}

GLImage::GLImage(GraphicsContext *context, VectorDataFormat format, glm::ivec3 size, int miplevels, ImageUsageBit usage, ImageType type) : m_size(size), 
	m_levels(miplevels), m_context(context), m_vec(format)
{
	m_target = typeToEnum[type];
	m_format = GetInternalFormatFromFormat(format);
}

GLImage::~GLImage()
{
	GLuint id = m_id;
	dynamic_cast<GLContext *>(m_context)->DeleteInitable([id]() {
		glDeleteTextures(1, &id);
	});
}

GraphicsImageView *GLImage::CreateView(ImageType type, VectorDataFormat format, int baseLevel, int levels, int baseLayer, int layers, ImageSwizzleComponent redSwizzle,
	ImageSwizzleComponent greenSizzle, ImageSwizzleComponent blueSwizzle, ImageSwizzleComponent alphaSwizzle)
{
	GLImageView *v = new GLImageView(m_context, this, type, format, baseLevel, levels, baseLayer, layers, redSwizzle, greenSizzle, blueSwizzle, alphaSwizzle);
	dynamic_cast<GLContext *>(m_context)->EnqueueInitable(v);
	return v;
}

void GLImage::InitInternal()
{
	glCreateTextures(m_target, 1, &m_id);
	switch (m_target)
	{
	case GL_TEXTURE_1D:
		glTextureStorage1D(m_id, m_levels, m_format, m_size.x);
		break;
	case GL_TEXTURE_1D_ARRAY:
	case GL_TEXTURE_2D:
	case GL_TEXTURE_CUBE_MAP:
		glTextureStorage2D(m_id, m_levels, m_format, m_size.x, m_size.y);
		break;
	case GL_TEXTURE_2D_ARRAY:
	case GL_TEXTURE_CUBE_MAP_ARRAY:
	case GL_TEXTURE_3D:
		glTextureStorage3D(m_id, m_levels, m_format, m_size.x, m_size.y, m_size.z);
		break;
	}
}

GLSampler::GLSampler(GraphicsContext *context, GraphicsSamplerState& state) : m_state(state), m_context(context)
{
}

GLSampler::~GLSampler()
{
	GLuint id = m_id;
	dynamic_cast<GLContext *>(m_context)->DeleteInitable([id]() {
		glDeleteSamplers(1, &id);
	});
}

void GLSampler::InitInternal()
{
	glCreateSamplers(1, &m_id);
	glSamplerParameterf(m_id, GL_TEXTURE_MAX_ANISOTROPY_EXT, m_state.AnisotropicFiltering ? 0 : m_state.MaxAnisotropy);
	if (m_state.UseBorderAsInteger)
	{
		int borderInt[] = { m_state.BorderColor.r, m_state.BorderColor.g, m_state.BorderColor.b, m_state.BorderColor.a };
		glSamplerParameteriv(m_id, GL_TEXTURE_BORDER_COLOR, borderInt);
	}
	else
	{
		float borderFloat[] = { m_state.BorderColor.r, m_state.BorderColor.g, m_state.BorderColor.b, m_state.BorderColor.a };
		glSamplerParameterfv(m_id, GL_TEXTURE_BORDER_COLOR, borderFloat);
	}
	glSamplerParameteri(m_id, GL_TEXTURE_COMPARE_MODE, m_state.DepthComparison);
	glSamplerParameteri(m_id, GL_TEXTURE_COMPARE_FUNC, compMode[m_state.DepthComparisonMode]);
	glSamplerParameterf(m_id, GL_TEXTURE_LOD_BIAS, m_state.LODBias);
	glSamplerParameterf(m_id, GL_TEXTURE_MIN_LOD, m_state.MinLOD);
	glSamplerParameterf(m_id, GL_TEXTURE_MAX_LOD, m_state.MaxLOD);
	glSamplerParameteri(m_id, GL_TEXTURE_MIN_FILTER, m_state.Minification == SamplerFilteringMode::Linear ?
		(m_state.EnableMipmapping ? (m_state.Mipmapping == SamplerFilteringMode::Linear ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR_MIPMAP_NEAREST) : GL_LINEAR)
		: (m_state.EnableMipmapping ? (m_state.Mipmapping == SamplerFilteringMode::Linear ? GL_NEAREST_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_NEAREST) : GL_NEAREST));
	glSamplerParameteri(m_id, GL_TEXTURE_MAG_FILTER, m_state.Magnification == SamplerFilteringMode::Linear ? GL_LINEAR : GL_NEAREST);
	glSamplerParameteri(m_id, GL_TEXTURE_WRAP_S, wrapMode[m_state.WrapU]);
	glSamplerParameteri(m_id, GL_TEXTURE_WRAP_T, wrapMode[m_state.WrapV]);
	glSamplerParameteri(m_id, GL_TEXTURE_WRAP_R, wrapMode[m_state.WrapW]);
}

GLImageView::GLImageView(GraphicsContext *context, GLImage *of, ImageType type, VectorDataFormat format, int baseLevel, int levels, int baseLayer, int layers, ImageSwizzleComponent redSwizzle,
	ImageSwizzleComponent greenSizzle, ImageSwizzleComponent blueSwizzle, ImageSwizzleComponent alphaSwizzle) : m_context(context), m_represented(of), m_baseLevel(baseLevel), m_levels(levels),
	m_baseLayer(baseLayer), m_layers(layers)
{
	m_target = typeToEnum[type];
	m_format = GetInternalFormatFromFormat(format);
	m_redSwizzle = redSwizzle;
	m_greenSizzle = greenSizzle;
	m_blueSwizzle = blueSwizzle;
	m_alphaSwizzle = alphaSwizzle;
}

GLImageView::~GLImageView()
{
	GLuint id = m_id;
	dynamic_cast<GLContext *>(m_context)->DeleteInitable([id]() {
		glDeleteTextures(1, &id);
	});
}

void GLImageView::InitInternal()
{
	glGenTextures(1, &m_id);
	glTextureView(m_id, m_target, m_represented->GetImageId(), m_format, m_baseLevel, m_levels, m_baseLayer, m_layers);

	GLint swizzleMask[] = { swizzleToInt[m_redSwizzle], swizzleToInt[m_greenSizzle], swizzleToInt[m_blueSwizzle], swizzleToInt[m_alphaSwizzle] };
	glTextureParameteriv(m_id, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
}

GLRenderTarget::GLRenderTarget(GraphicsContext *context, std::vector<GraphicsImageView *>&& attachments, GraphicsImageView *depthStencil, int width, int height, int layers) : m_width(width), m_height(height), m_layers(layers), 
	m_context(context), m_attachments(attachments), m_depthStencil(depthStencil)
{
	m_colorAtts = attachments.size();
}

GLRenderTarget::~GLRenderTarget()
{
	GLuint id = m_id;
	dynamic_cast<GLContext *>(m_context)->DeleteInitable([id]() {
		glDeleteFramebuffers(1, &id);
	});
}

void GLRenderTarget::InitInternal()
{
	glCreateFramebuffers(1, &m_id);
	int index = 0;
	for (GraphicsImageView *view : m_attachments)
	{
		GLImageView *v = dynamic_cast<GLImageView *>(view);
		glNamedFramebufferTexture(m_id, GL_COLOR_ATTACHMENT0 + index, v->GetViewId(), 0);
		++index;
	}
	if (m_depthStencil)
	{
		GLImageView *view = dynamic_cast<GLImageView *>(m_depthStencil);
		GLenum attachmentType = 0;
		switch (view->GetFormat())
		{
		case GL_STENCIL_INDEX8:
			attachmentType = GL_STENCIL_ATTACHMENT;
			break;
		case GL_DEPTH24_STENCIL8:
		case GL_DEPTH32F_STENCIL8:
			attachmentType = GL_DEPTH_STENCIL_ATTACHMENT;
			m_isDepthStencil = true;
			break;
		default:
			attachmentType = GL_DEPTH_ATTACHMENT;
			break;
		}
		glNamedFramebufferTexture(m_id, attachmentType, view->GetViewId(), 0);
	}
	glNamedFramebufferParameteri(m_id, GL_FRAMEBUFFER_DEFAULT_WIDTH, m_width);
	glNamedFramebufferParameteri(m_id, GL_FRAMEBUFFER_DEFAULT_HEIGHT, m_height);
	glNamedFramebufferParameteri(m_id, GL_FRAMEBUFFER_DEFAULT_LAYERS, m_layers);
}

GLRenderPass::GLRenderPass(GraphicsRenderPassState& state)
{
	GraphicsRenderPassLoadStore loadDepth = state.LoadOperations.back();
	GraphicsRenderPassLoadStore loadStencil = state.StencilLoadOperation;
	GraphicsRenderPassLoadStore storeDepth = state.StoreOperations.back();
	GraphicsRenderPassLoadStore storeStencil = state.StencilStoreOperation;

	m_clearDepth = loadDepth == GraphicsRenderPassLoadStore::Clear;
	m_clearStencil = loadStencil == GraphicsRenderPassLoadStore::Clear;

	if (loadDepth == GraphicsRenderPassLoadStore::Discard)
		m_discard.push_back(GL_DEPTH_ATTACHMENT);
	if (loadStencil == GraphicsRenderPassLoadStore::Discard)
		m_discard.push_back(GL_STENCIL_ATTACHMENT);
	if (storeDepth == GraphicsRenderPassLoadStore::Discard)
		m_discardAfter.push_back(GL_DEPTH_ATTACHMENT);
	if (storeStencil == GraphicsRenderPassLoadStore::Discard)
		m_discardAfter.push_back(GL_STENCIL_ATTACHMENT);

	for (int i = 0; i < state.LoadOperations.size() - 1; ++i)
	{
		GraphicsRenderPassLoadStore load = state.LoadOperations[i];
		GraphicsRenderPassLoadStore store = state.StoreOperations[i];

		if (load == GraphicsRenderPassLoadStore::Clear)
			m_clearColors.push_back(i);
		else if (load == GraphicsRenderPassLoadStore::Discard)
			m_discard.push_back(GL_COLOR_ATTACHMENT0 + i);
		if (store == GraphicsRenderPassLoadStore::Discard)
			m_discardAfter.push_back(GL_COLOR_ATTACHMENT0 + i);
	}

	for (GraphicsRenderSubpassState& subpass : state.Subpasses)
	{
		GLSubpassPerform perf;
		if (subpass.InputAttachments.size() != 0)
			perf.DoTextureBarrier = true;
		else if (subpass.ColorAttachments.size() == 0)
			perf.DoNoneAttachments = true;
		else
		{
			for (int i : subpass.ColorAttachments)
				perf.enums.push_back(GL_COLOR_ATTACHMENT0 + i);
		}
		m_subpassPerform.push_back(perf);
	}
}

void GLRenderPass::PerformPreOps(std::vector<glm::vec4>& attachmentClearValues, int stencil)
{
	glClearBufferfi(GL_DEPTH_STENCIL, 0, attachmentClearValues.back().r, stencil);
	for (int i : m_clearColors)
		glClearBufferfv(GL_COLOR, i, &attachmentClearValues[i][0]); 
	/*
	if (m_clearDepth)
		glClearBufferfi(GL_DEPTH, 0, attachmentClearValues.back().r, 0);
	if (m_clearStencil)
		glClearBufferfi(GL_STENCIL, 0, 9, stencil);*/

	glInvalidateFramebuffer(GL_DRAW_FRAMEBUFFER, m_discard.size(), m_discard.data());
}

void GLRenderPass::BindSubpass(int i)
{
	GLSubpassPerform& perform = m_subpassPerform[i];
	if (perform.DoTextureBarrier)
		glTextureBarrier();
	if (perform.DoNoneAttachments)
		glDrawBuffer(GL_NONE);
	else
	{
		glDrawBuffers(perform.enums.size(), perform.enums.data());
	}
}

void GLRenderPass::PerformPostOps()
{
	glInvalidateFramebuffer(GL_DRAW_FRAMEBUFFER, m_discardAfter.size(), m_discardAfter.data());
}