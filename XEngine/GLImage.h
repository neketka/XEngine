#pragma once

#include "GraphicsDefs.h"
#include "GLInitable.h"

#include <GL/glew.h>
#include <map>
#include <queue>

class VectorFormatDesc
{
public:
	VectorFormatDesc(int32_t r, int32_t g, int32_t b, int32_t a, bool integer, bool norm, bool unsign, bool srgb, bool lon, bool depth, bool stencil)
		: RBits(r), GBits(g), BBits(b), ABits(a), Integer(integer), Normalized(norm), Unsigned(unsign), SRGB(unsign), Long(lon),
	Depth(depth), Stencil(stencil) {}

	int32_t RBits;
	int32_t GBits;
	int32_t BBits;
	int32_t ABits;
	bool Integer;
	bool Normalized;
	bool Unsigned;
	bool SRGB;
	bool Long;
	bool Depth;
	bool Stencil;
};

VectorFormatDesc GetFormatDesc(VectorDataFormat format);
GLenum GetInternalFormatFromFormat(VectorDataFormat format);

class GLImageView;
class GLSampler : public GraphicsSampler, public GLInitable
{
public:
	GLSampler(GraphicsContext *context, GraphicsSamplerState& state);
	GLuint GetSamplerId() { return m_id; }
	virtual ~GLSampler() override;
private:
	GraphicsContext *m_context;
	GLuint m_id;
	GraphicsSamplerState m_state;

	virtual void InitInternal() override;
};

class GLImage : public GraphicsImageObject, public GLInitable
{
public:
	GLImage(GraphicsContext *context, VectorDataFormat format, glm::ivec3 size, int32_t miplevels, ImageUsageBit usage, ImageType type);
	virtual ~GLImage() override;
	virtual GraphicsImageView *CreateView(ImageType type, VectorDataFormat format, int32_t baseLevel, int32_t levels, int32_t baseLayer, int32_t layers, ImageSwizzleComponent redSwizzle, 
		ImageSwizzleComponent greenSizzle, ImageSwizzleComponent blueSwizzle, ImageSwizzleComponent alphaSwizzle) override;

	int32_t GetLevels() { return m_levels; }
	glm::ivec3 GetSize() { return m_size; }
	GLenum GetFormat() { return m_format; }
	VectorDataFormat GetVectorFormat() { return m_vec; }
	GLenum GetTarget() { return m_target; }
	GLuint GetImageId() { return m_id; }
private:
	GraphicsContext *m_context;
	int32_t m_levels;
	glm::ivec3 m_size;
	VectorDataFormat m_vec;
	GLenum m_format;
	GLenum m_target;
	GLuint m_id;

	// Inherited via GLInitable
	virtual void InitInternal() override;
};

class GLImageView : public GraphicsImageView, public GLInitable
{
public:
	GLImageView(GraphicsContext *context, GLImage *of, ImageType type, VectorDataFormat format, int32_t baseLevel, int32_t levels, int32_t baseLayer, int32_t layers, ImageSwizzleComponent redSwizzle,
		ImageSwizzleComponent greenSizzle, ImageSwizzleComponent blueSwizzle, ImageSwizzleComponent alphaSwizzle);
	virtual ~GLImageView() override;
	GLuint GetViewId() { return m_id; }
	GLenum GetFormat() { return m_format; }
private:
	GraphicsContext *m_context;
	GLImage *m_represented;
	ImageSwizzleComponent m_redSwizzle;
	ImageSwizzleComponent m_greenSizzle;
	ImageSwizzleComponent m_blueSwizzle;
	ImageSwizzleComponent m_alphaSwizzle;
	int32_t m_baseLevel;
	int32_t m_levels;
	int32_t m_baseLayer;
	int32_t m_layers;
	GLenum m_format;
	GLenum m_target;
	GLuint m_id;

	// Inherited via GLInitable
	virtual void InitInternal() override;
};

class GLRenderTarget : public GraphicsRenderTarget, public GLInitable
{
public:
	GLRenderTarget(GraphicsContext *context, std::vector<GraphicsImageView *>&& attachments, GraphicsImageView *depthStencil, int32_t width, int32_t height, int32_t layers);
	virtual ~GLRenderTarget() override;
	GLuint GetFBOId() { return m_id; }
	int32_t GetWidth() { return m_width; }
	int32_t GetHeight() { return m_height; }
	int32_t GetColorAttachmentCount() { return m_colorAtts; }
	bool IsDepthStencil() { return m_isDepthStencil; }
private:
	GraphicsContext *m_context;
	std::vector<GraphicsImageView *> m_attachments;
	GraphicsImageView *m_depthStencil;
	bool m_isDepthStencil = false;
	int32_t m_width, m_height, m_layers, m_colorAtts;
	GLuint m_id;
	virtual void InitInternal() override;
};

class GLSubpassPerform
{
public:
	bool DoTextureBarrier = false;
	bool DoNoneAttachments = false;
	std::vector<GLenum> enums;
};

class GLRenderPass : public GraphicsRenderPass 
{
public:
	GLRenderPass(GraphicsRenderPassState& state);
	void PerformPreOps(std::vector<glm::vec4>& attachmentClearValues, int32_t stencil);
	void BindSubpass(int32_t i);
	void PerformPostOps();
private:
	bool m_clearDepth;
	bool m_clearStencil;
	std::vector<int32_t> m_clearColors;
	std::vector<GLenum> m_discard;
	std::vector<GLenum> m_discardAfter;
	std::vector<GLSubpassPerform> m_subpassPerform;
};