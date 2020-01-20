#pragma once
#include <GL\glew.h>
#include <map>
#include <queue>

#include "GLResource.h"
#include "Sampler.h"

class ImageUnitManager
{
public:
	static void Initialize(GLint availableUnits);
	static int RefreshUnit(GLint id);
private:
	static std::map<GLint, GLint> m_availableUnits;
	static std::queue<GLint> m_ageSort;
};

enum class TextureInternalFormat
{
	Depth16I = GL_DEPTH_COMPONENT16, Depth24I = GL_DEPTH_COMPONENT24, Depth32I = GL_DEPTH_COMPONENT32, Depth32F = GL_DEPTH_COMPONENT32F, 
	Depth24IStencil8UI = GL_DEPTH24_STENCIL8, Depth32FStencil8UI = GL_DEPTH32F_STENCIL8,
	Stencil8UI = GL_STENCIL_INDEX8,
	R8 = GL_R8, RG8 = GL_RG8, RGB8 = GL_RGB8, RGBA8 = GL_RGBA8,
	R8I = GL_R8I, RG8I = GL_RG8I, RGB8I = GL_RGB8I, RGBA8I = GL_RGBA8I, 
	R16I = GL_R16I, RG16I = GL_RG16I, RGB16I = GL_RGB16I, RGBA16I = GL_RGBA16I,
	R16F = GL_R16F, RG16F = GL_RG16F, RGB16F = GL_RGB16F, RGBA16F = GL_RGBA16F,
	R32F = GL_R32F, RG32F = GL_RG32F, RGB32F = GL_RGB32F, RGBA32F = GL_RGBA32F
};

enum class TexturePixelFormat
{
	R = GL_RED, RG = GL_RG, RGB = GL_RGB, BGR = GL_BGR, RGBA = GL_RGBA, 
	BGRA = GL_BGRA, Depth = GL_DEPTH_COMPONENT, Stencil = GL_STENCIL_INDEX
};

enum class CubemapSide
{
	PositiveX, NegativeX, PositiveY, NegativeY, PositiveZ, NegativeZ
};

enum class TextureDataType
{
	UnsignedByte = GL_UNSIGNED_BYTE, UnsignedInt = GL_UNSIGNED_INT, Int = GL_INT, Float = GL_FLOAT
};

int GetByteDepth(TexturePixelFormat format, TextureDataType type);

class Image
{
public:
	Image(GLint iCreated, GLuint tex, GLint level, bool layered, GLenum layer, GLenum access, TextureInternalFormat format) :
		m_iCreated(iCreated), m_tex(tex), m_level(level), m_layered(layered), m_layer(layer), m_access(access), m_format(format) {}
	int BindToUnit() 
	{
		auto unit = ImageUnitManager::RefreshUnit(m_iCreated);
		glBindImageTexture(unit, m_tex, m_level, m_layered, m_layer, m_access, static_cast<GLenum>(m_format));
		return unit;
	}
private:
	GLuint m_tex; 
	GLint m_level;
	bool m_layered; 
	GLenum m_layer;
	GLenum m_access; 
	TextureInternalFormat m_format;
	GLint m_iCreated;
};

class WrappedTextureView
{
public:
	WrappedTextureView() { }
	WrappedTextureView(int layer, int level, TextureInternalFormat format, GLuint id)
		: m_layer(layer), m_level(level), m_format(format), m_id(id) {}

	inline int GetLayer() { return m_layer; }
	inline int GetLevel() { return m_level; }
	inline TextureInternalFormat GetFormat() { return m_format; }
	inline GLuint GetID() { return m_id; }

private:
	int m_layer = 0;
	int m_level = 0;
	TextureInternalFormat m_format = TextureInternalFormat::R8;
	GLuint m_id = 0;
};

class Texture : public IGLResource
{
public:
	Texture() {}
	Texture(int width, int height, bool isCubemap, int levels, TextureInternalFormat format);
	Texture(int width, int height, int depth, int levels, TextureInternalFormat format);
	Texture(int width, int height, int slices, bool isCubemapArray, int levels, TextureInternalFormat format);

	Texture(Texture viewTarget, int baseLevel, int baseSlice, int width, int height, bool isCubemap, int levels, TextureInternalFormat format) : Texture(width, height, isCubemap, levels, format)
	{
		glTextureView(m_id, m_type, viewTarget.GetID(), static_cast<GLenum>(format), baseLevel, levels, baseSlice, isCubemap ? 6 : 1);
	}

	Texture(Texture viewTarget, int baseLevel, int baseSlice, int width, int height, int depth, int levels, TextureInternalFormat format) : Texture(width, height, depth, levels, format)
	{
		glTextureView(m_id, m_type, viewTarget.GetID(), static_cast<GLenum>(format), baseLevel, levels, baseSlice, depth);
	}

	Texture(Texture viewTarget, int baseLevel, int baseSlice, int width, int height, int slices, bool isCubemapArray, int levels, TextureInternalFormat format) : Texture(width, height, slices, isCubemapArray, levels, format)
	{
		glTextureView(m_id, m_type, viewTarget.GetID(), static_cast<GLenum>(format), baseLevel, levels, baseSlice, m_d);
	}

	void GenerateMipmap();
	void SetData(int x, int y, int w, int h, int level, TexturePixelFormat format, TextureDataType type, const void *pixels);
	void SetData(int x, int y, int z, int w, int h, int d, int level, TexturePixelFormat format, TextureDataType type, const void *pixels);
	void CopyPixels(Texture dest, int sX, int sY, int sZ, int sLevel, int dX, int dY, int dZ, int dLevel, int width, int height, int depth);
	void ReadPixels(int x, int y, int z, int w, int h, int d, int level, TexturePixelFormat format, TextureDataType type, int bufferSize, void *pixels);
	int BindToSampler(Sampler sampler);

	Image CreateImage(TextureInternalFormat format, int level, bool read, bool write);
	Image CreateImage(TextureInternalFormat format, int level, bool read, bool write, int layer);

	void SetMipmapRange(int base, int max);

	WrappedTextureView CreateWrappedView(int layer, int level);

	void Destroy() override;

	inline GLuint GetID() { return m_id; }
	inline TextureInternalFormat GetFormat() { return m_format; }

	inline int GetWidth() { return m_w; }
	inline int GetHeight() { return m_h; }
	inline int GetDepth() { return m_h; }
private:
	int m_w = 0, m_h = 0, m_d = 0;
	static int m_imagesMade;
	GLuint m_id = 0;
	GLenum m_type = 0;
	TextureInternalFormat m_format = TextureInternalFormat::R8;
};