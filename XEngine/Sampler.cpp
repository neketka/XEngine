#include "Sampler.h"

std::map<GLint, GLint> TextureUnitManager::m_availableUnits;
std::queue<GLint> TextureUnitManager::m_ageSort;

void TextureUnitManager::Initialize(GLint availableUnits)
{
	for (GLint i = 0; i < availableUnits; ++i)
	{
		m_availableUnits.insert({ i - availableUnits, i });
		m_ageSort.push(i - availableUnits);
	}
}

int TextureUnitManager::RefreshUnit(GLint id)
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

Sampler::Sampler()
{
	glCreateSamplers(1, &m_id);
}

void Sampler::SetWrapMode(TextureChannel channel, WrapMode mode)
{
	glSamplerParameteri(m_id, static_cast<GLenum>(channel), static_cast<GLenum>(mode));
	switch (channel)
	{
	case TextureChannel::S:
		m_sWrap = mode;
		break;
	case TextureChannel::T:
		m_tWrap = mode;
		break;
	case TextureChannel::R:
		m_rWrap = mode;
		break;
	}
}

void Sampler::SetMinificationFilter(MinificationFilter filter)
{
	glSamplerParameteri(m_id, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(filter));
	m_minFilter = filter;
}

void Sampler::SetMagnificationFilter(MagnificationFilter filter)
{
	glSamplerParameteri(m_id, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(filter));
	m_magFilter = filter;
}

void Sampler::SetCompareMode(bool enabled)
{
	glSamplerParameteri(m_id, GL_TEXTURE_COMPARE_MODE, enabled ? GL_COMPARE_REF_TO_TEXTURE : GL_NONE);
	m_depthCompare = enabled;
}

void Sampler::SetComparison(ComparisonFunc func)
{
	glSamplerParameteri(m_id, GL_TEXTURE_COMPARE_FUNC, static_cast<GLint>(func));
	m_compareFunc = func;
}

void Sampler::SetAnisotropicFiltering(bool enabled)
{
	float aniso = 1.0f;
	if (enabled)
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &aniso);
	glSamplerParameterf(m_id, GL_TEXTURE_MAX_ANISOTROPY, aniso);
	m_anisotropicFiltering = enabled;
}

void Sampler::SetBorderColor(glm::vec4 color)
{
	glSamplerParameterfv(m_id, GL_TEXTURE_BORDER_COLOR, reinterpret_cast<float *>(&color));
	m_borderColor = color;
}

int Sampler::BindToTextureUnit()
{
	int unit = TextureUnitManager::RefreshUnit(m_id);
	glBindSampler(unit, m_id);
	return unit;
}

void Sampler::Destroy()
{
	if (m_id != 0)
		glDeleteSamplers(1, &m_id);
	m_id = 0;
}
