#include "pch.h"
#include "GLShaderData.h"
#include "GLContext.h"

GLuint GLShader::GetSpecialization(GLShaderSpecialization *specialization)
{
	GLuint shader = glCreateShader(ConvertFromShaderStage(m_code.GetStage()));
	GLuint program = glCreateProgram();

	std::vector<GLuint> locs(specialization->GetConstants().size());
	std::vector<GLuint> values(specialization->GetConstants().size());

	for (auto [loc, val] : specialization->GetConstants())
	{
		locs.push_back(loc);
		values.push_back(val);
	}

	glProgramParameteri(program, GL_PROGRAM_SEPARABLE, GL_TRUE);
	glShaderBinary(1, &shader, GL_SHADER_BINARY_FORMAT_SPIR_V_ARB, m_code.GetCode().data(), m_code.GetCode().size() * sizeof(uint32_t));
	glSpecializeShaderARB(shader, specialization->GetEntryPoint().c_str(), locs.size(), locs.data(), values.data());
	glAttachShader(program, shader);
	glLinkProgram(program);
	glDeleteShader(shader);

	GLsizei len = 0;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
	char *log0 = new char[len];
	glGetShaderInfoLog(shader, len, nullptr, log0);

	len = 0;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
	char *log = new char[len];
	glGetProgramInfoLog(program, len, nullptr, log);
	
	delete[] log0;
	delete[] log;

	//TODO: Error checking
	
	std::set<std::pair<uint32_t, uint32_t>> st(specialization->GetConstants().begin(), specialization->GetConstants().end());

	m_specializations[st] = program;
	return program;
}

void GLShader::ClearSpecializations()
{
	std::map<std::set<std::pair<GLuint, GLuint>>, GLuint> specializations = m_specializations;
	dynamic_cast<GLContext *>(m_context)->DeleteInitable([specializations]() {
		for (auto kp : specializations)
			glDeleteProgram(kp.second);
		});
	m_specializations.clear();
}

GLShader::~GLShader()
{
	std::map<std::set<std::pair<GLuint, GLuint>>, GLuint> specializations = m_specializations;
	dynamic_cast<GLContext *>(m_context)->DeleteInitable([specializations]() {
		for (auto kp : specializations)
			glDeleteProgram(kp.second);
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
