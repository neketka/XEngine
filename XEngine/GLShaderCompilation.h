#pragma once
#include "GraphicsDefs.h"
#include <set>

class GLShaderCode : public GraphicsSpecificShaderCode
{
public:
	GLShaderCode(std::vector<uint32_t> code, ShaderStageBit stage) : m_code(code), m_stage(stage) {}
	std::vector<uint32_t>& GetCode() { return m_code; }
	ShaderStageBit& GetStage() { return m_stage; }
private:
	std::vector<uint32_t> m_code;
	ShaderStageBit m_stage;
};

class GLShaderSpecialization : public GraphicsSpecificSpecializationData
{
public:
	GLShaderSpecialization(std::string entryPoint, std::vector<std::pair<uint32_t, uint32_t>> defines)
		: m_defines(defines), m_entryPoint(entryPoint) {}
	std::vector<std::pair<uint32_t, uint32_t>>& GetConstants() { return m_defines; }
	std::string& GetEntryPoint() { return m_entryPoint; }
private:
	std::vector<std::pair<uint32_t, uint32_t>> m_defines;
	std::string m_entryPoint;
};