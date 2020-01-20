#pragma once
#include <string>
#include <gl\glew.h>
#include <memory>
#include "GLResource.h"
#include <vector>
#include <set>

class Shader : public IGLResource
{
public:
	Shader() {}
	Shader(std::string src, std::set<std::string> defines);
	inline GLuint GetID() const { return m_id; }
	inline std::string GetShaderInfoLog() { return m_infoLog; }
	inline std::vector<std::string> GetShaderStageInfoLogs() { return m_shaderInfoLogs; }
	inline bool IsCompileSuccess() { return m_compileSuccess; }
	virtual void Destroy() override;
private:
	bool m_compileSuccess = false;
	std::string m_infoLog;
	std::vector<std::string> m_shaderInfoLogs;
	GLuint m_id = 0;
};
