#include "pch.h"
#include "GLShaderData.h"
#include "GLContext.h"

GLuint GLShader::GetSpecialization(GLShaderSpecialization *specialization)
{
	auto found = m_specializations.find(specialization->GetDefines());
	if (found != m_specializations.end())
		return found->second;

	std::vector<std::string> sBuffers;
	std::vector<const char *> strings;
	sBuffers.reserve(specialization->GetDefines().size() + 3);
	strings.reserve(specialization->GetDefines().size() + 3);
	strings.push_back("#version 460\n");
	for (std::string specs : specialization->GetDefines())
	{
		sBuffers.push_back("#define " + specs + "\n");
		strings.push_back(sBuffers.back().c_str());
	}
	strings.push_back("#line 0\n");
	strings.push_back(m_code.GetCode().c_str());

	GLuint shader = glCreateShaderProgramv(ConvertFromShaderStage(m_code.GetStage()), strings.size(), strings.data());

	GLsizei len = 0;
	glGetProgramiv(shader, GL_INFO_LOG_LENGTH, &len);
	char *log = new char[len];
	glGetProgramInfoLog(shader, len, nullptr, log);
	delete[] log;

	//TODO: Error checking

	m_specializations[specialization->GetDefines()] = shader;
	return shader;
}

GLShader::~GLShader()
{
	std::map<std::set<std::string>, GLuint> specializations = m_specializations;
	dynamic_cast<GLContext *>(m_context)->DeleteInitable([specializations]() {
		for (auto kp : specializations)
			glDeleteShader(kp.second);
	});
}

GLenum ConvertFromShaderStageBit(ShaderStageBit bits)
{
	return (bits & ShaderStageBit::Vertex ? GL_VERTEX_SHADER_BIT : 0) |
		(bits & ShaderStageBit::Compute ? GL_COMPUTE_SHADER_BIT : 0) |
		(bits & ShaderStageBit::Fragment ? GL_FRAGMENT_SHADER_BIT : 0) |
		(bits & ShaderStageBit::Geometry ? GL_GEOMETRY_SHADER_BIT : 0) |
		(bits & ShaderStageBit::TessControl ? GL_TESS_CONTROL_SHADER_BIT : 0) |
		(bits & ShaderStageBit::TessEval ? GL_TESS_EVALUATION_SHADER_BIT : 0);
}

std::map< ShaderStageBit, GLenum> stageToEnum = {
	{ ShaderStageBit::Vertex, GL_VERTEX_SHADER },
	{ ShaderStageBit::Compute, GL_COMPUTE_SHADER },
	{ ShaderStageBit::Fragment, GL_FRAGMENT_SHADER },
	{ ShaderStageBit::Geometry, GL_GEOMETRY_SHADER },
	{ ShaderStageBit::TessControl, GL_TESS_CONTROL_SHADER },
	{ ShaderStageBit::TessEval, GL_TESS_EVALUATION_SHADER }
};

GLenum ConvertFromShaderStage(ShaderStageBit stage)
{
	return stageToEnum[stage];
}

GLShaderResourceInstance::GLShaderResourceInstance(GraphicsShaderResourceViewData& data) : m_data(data)
{
}

void GLShaderResourceInstance::InitInternal()
{
	m_viewIds.reserve(m_views.size());
	m_samplerIds.reserve(m_data.Type == ShaderResourceType::InputAttachment ? m_views.size() : m_samplerIds.size());
	for (GraphicsImageView *view : m_views)
	{
		if (m_data.Type == ShaderResourceType::InputAttachment)
			m_samplerIds.push_back(0);
		m_viewIds.push_back(dynamic_cast<GLImageView *>(view)->GetViewId());
	}
	for (GraphicsSampler *sampler : m_samplers)
		m_samplerIds.push_back(dynamic_cast<GLSampler *>(sampler)->GetSamplerId());
}
