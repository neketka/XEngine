#pragma once
#include "GraphicsDefs.h"

class GLShaderCode : public GraphicsSpecificShaderCode
{
public:
	GLShaderCode(std::string code, ShaderStageBit stage) : m_code(code), m_stage(stage) {}
	std::string& GetCode() { return m_code; }
	ShaderStageBit& GetStage() { return m_stage; }
private:
	std::string m_code;
	ShaderStageBit m_stage;
};

class GLShaderSpecialization : public GraphicsSpecificSpecializationData
{
public:
	GLShaderSpecialization(std::set<std::string>&& defines) : m_defines(defines) {}
	std::set<std::string>& GetDefines() { return m_defines; }
private:
	std::set<std::string> m_defines;
};