#pragma once

#include "GraphicsDefs.h"
#include "GLInitable.h"

#include <GL/glew.h>
#include <map>
#include <queue>

class VectorFormatDesc
{
public:
	VectorFormatDesc(int r, int g, int b, int a, bool integer, bool norm, bool unsign, bool srgb, bool lon, bool depth, bool stencil)
		: RBits(r), GBits(g), BBits(b), ABits(a), Integer(integer), Normalized(norm), Unsigned(unsign), SRGB(unsign), Long(lon),
	Depth(depth), Stencil(stencil) {}

	int RBits;
	int GBits;
	int BBits;
	int ABits;
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
	GLImage(GraphicsContext *context, VectorDataFormat format, glm::ivec3 size, int miplevels, ImageUsageBit usage, ImageType type);
	virtual ~GLImage() override;
	virtual GraphicsImageView *CreateView(ImageType type, VectorDataFormat format, int baseLevel, int levels, int baseLayer, int layers, ImageSwizzleComponent redSwizzle, 
		ImageSwizzleComponent greenSizzle, ImageSwizzleComponent blueSwizzle, ImageSwizzleComponent alphaSwizzle) override;

	int GetLevels() { return m_levels; }
	glm::ivec3 GetSize() { return m_size; }
	GLenum GetFormat() { return m_format; }
	VectorDataFormat GetVectorFormat() { return m_vec; }
	GLenum GetTarget() { return m_target; }
	GLuint GetImageId() { return m_id; }
private:
	GraphicsContext *m_context;
	int m_levels;
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
	GLImageView(GraphicsContext *context, GLImage *of, ImageType type, VectorDataFormat format, int baseLevel, int levels, int baseLayer, int layers, ImageSwizzleComponent redSwizzle,
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
	int m_baseLevel;
	int m_levels;
	int m_baseLayer;
	int m_layers;
	GLenum m_format;
	GLenum m_target;
	GLuint m_id;

	// Inherited via GLInitable
	virtual void InitInternal() override;
};

class GLRenderTarget : public GraphicsRenderTarget, public GLInitable
{
public:
	GLRenderTarget(GraphicsContext *context, std::vector<GraphicsImageView *>&& attachments, GraphicsImageView *depthStencil, int width, int height, int layers);
	virtual ~GLRenderTarget() override;
	GLuint GetFBOId() { return m_id; }
	int GetWidth() { return m_width; }
	int GetHeight() { return m_height; }
	int GetColorAttachmentCount() { return m_colorAtts; }
	bool IsDepthStencil() { return m_isDepthStencil; }
private:
	GraphicsContext *m_context;
	std::vector<GraphicsImageView *> m_attachments;
	GraphicsImageView *m_depthStencil;
	bool m_isDepthStencil = false;
	int m_width, m_height, m_layers, m_colorAtts;
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
	void PerformPreOps(std::vector<glm::vec4>& attachmentClearValues, int stencil);
	void BindSubpass(int i);
	void PerformPostOps();
private:
	bool m_clearDepth;
	bool m_clearStencil;
	std::vector<int> m_clearColors;
	std::vector<GLenum> m_discard;
	std::vector<GLenum> m_discardAfter;
	std::vector<GLSubpassPerform> m_subpassPerform;
};