#include "Texture.h"

std::map<GLint, GLint> ImageUnitManager::m_availableUnits;
std::queue<GLint> ImageUnitManager::m_ageSort;

void ImageUnitManager::Initialize(GLint availableUnits)
{
	for (GLint i = 0; i < availableUnits; ++i)
	{
		m_availableUnits.insert({ i - availableUnits, i });
		m_ageSort.push(i - availableUnits);
	}
}

int ImageUnitManager::RefreshUnit(GLint id)
{
	if (m_availableUnits.find(id) != m_availableUnits.end())
		return m_availableUnits[id];
	else
	{
		GLint oldId = m_ageSort.front();
		m_ageSort.pop();
		GLint unit = m_availableUnits[oldId];
		m_availableUnits.erase(oldId);
		m_availableUnits.insert({ id, unit });
		m_ageSort.push(id);
		return unit;
	}
}

Texture::Texture(int w, int h, bool isCubemap, int levels, TextureInternalFormat format) : m_format(format), m_w(w), m_h(h)
{
	if (isCubemap)
		m_type = GL_TEXTURE_CUBE_MAP;
	else
		m_type = GL_TEXTURE_2D;

	m_w = w, m_h = h, m_d = 1;
	glCreateTextures(m_type, 1, &m_id);
	glTextureStorage2D(m_id, levels, static_cast<GLenum>(format), w, h);
	glTextureParameteri(m_id, GL_TEXTURE_BASE_LEVEL, 0);
	glTextureParameteri(m_id, GL_TEXTURE_MAX_LEVEL, levels - 1);
}

Texture::Texture(int width, int height, int depth, int levels, TextureInternalFormat format) : m_format(format), m_w(width), m_h(height)
{
	m_w = width, m_h = height, m_d = depth;
	m_type = GL_TEXTURE_3D;
	glCreateTextures(GL_TEXTURE_3D, 1, &m_id);
	glTextureStorage3D(m_id, levels, static_cast<GLenum>(format), width, height, depth);
	glTextureParameteri(m_id, GL_TEXTURE_BASE_LEVEL, 0);
	glTextureParameteri(m_id, GL_TEXTURE_MAX_LEVEL, levels - 1);
}

Texture::Texture(int width, int height, int slices, bool isCubemapArray, int levels, TextureInternalFormat format) : m_format(format), m_w(width), m_h(height)
{
	if (isCubemapArray)
	{
		slices *= 6;
		m_type = GL_TEXTURE_CUBE_MAP_ARRAY;
	}
	else
		m_type = GL_TEXTURE_2D_ARRAY;
	m_w = width, m_h = height, m_d = slices;
	glCreateTextures(m_type, 1, &m_id);
	glTextureStorage3D(m_id, levels, static_cast<GLenum>(format), width, height, slices);
	glTextureParameteri(m_id, GL_TEXTURE_BASE_LEVEL, 0);
	glTextureParameteri(m_id, GL_TEXTURE_MAX_LEVEL, levels - 1);
}

void Texture::SetData(int x, int y, int w, int h, int level, TexturePixelFormat format, TextureDataType type, const void *pixels)
{
	glTextureSubImage2D(m_id, level, x, y, w, h, static_cast<GLenum>(format), static_cast<GLenum>(type), pixels);
}

void Texture::SetData(int x, int y, int z, int w, int h, int d, int level, TexturePixelFormat format, TextureDataType type, const void * pixels)
{
	glTextureSubImage3D(m_id, level, x, y, z, w, h, d, static_cast<GLenum>(format), static_cast<GLenum>(type), pixels);
}

void Texture::CopyPixels(Texture dest, int sX, int sY, int sZ, int sLevel, int dX, int dY, int dZ, int dLevel, int width, int height, int depth)
{
	glCopyImageSubData(m_id, m_type, sLevel, sX, sY, sZ, dest.m_id, dest.m_type, dLevel, dX, dY, dZ, width, height, depth);
}

void Texture::ReadPixels(int x, int y, int z, int w, int h, int d, int level, TexturePixelFormat format, TextureDataType type, int bufferSize, void* pixels)
{
	glGetTextureSubImage(m_id, level, x, y, z, w, h, d, static_cast<GLenum>(format), static_cast<GLenum>(type), bufferSize, pixels);
}

void Texture::Destroy()
{
	if (m_id != 0)
		glDeleteTextures(1, &m_id);
	m_id = 0;
}

int Texture::m_imagesMade;

Image Texture::CreateImage(TextureInternalFormat format, int level, bool read, bool write)
{
	return Image(m_imagesMade++, m_id, level, false, 0, read ? write ? GL_READ_WRITE : GL_WRITE_ONLY : GL_READ_ONLY, format);
}

Image Texture::CreateImage(TextureInternalFormat format, int level, bool read, bool write, int layer)
{
	return Image(m_imagesMade++, m_id, level, true, layer, read ? write ? GL_READ_WRITE : GL_WRITE_ONLY : GL_READ_ONLY, format);
}

void Texture::SetMipmapRange(int base, int max)
{
	glTextureParameteri(m_id, GL_TEXTURE_BASE_LEVEL, base);
	glTextureParameteri(m_id, GL_TEXTURE_MAX_LEVEL, max);
}

WrappedTextureView Texture::CreateWrappedView(int layer, int level)
{
	return WrappedTextureView(layer, level, m_format, m_id);
}

void Texture::GenerateMipmap()
{
	glGenerateTextureMipmap(m_id);
}

int Texture::BindToSampler(Sampler sampler)
{
	int unit = sampler.BindToTextureUnit();
	glBindTextureUnit(unit, m_id);
	return unit;
}


int GetByteDepth(TexturePixelFormat format, TextureDataType type) 
{
	int base = 1;
	switch (type)
	{
	case TextureDataType::UnsignedByte:
		base *= 1;
		break;
	case TextureDataType::UnsignedInt:
		base *= 4;
		break;
	case TextureDataType::Int:
		base *= 4;
		break;
	case TextureDataType::Float:
		base *= 4;
		break;
	}

	switch (format)
	{
	case TexturePixelFormat::R:
		base *= 1;
		break;
	case TexturePixelFormat::RG:
		base *= 2;
		break;
	case TexturePixelFormat::RGB:
		base *= 3;
		break;
	case TexturePixelFormat::RGBA:
	case TexturePixelFormat::BGRA:
		base *= 4;
		break;
	}
	return base;
}
