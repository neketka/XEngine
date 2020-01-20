#pragma once
#include <glm\glm.hpp>

#include "Texture.h"

class ShaderConstants
{
public:
	void AddConstant(int location, Texture texture, Sampler sampler);
	void AddConstant(int location, Image image);
	void AddConstant(int location, int i);
	void AddConstant(int location, float f);
	void AddConstant(int location, glm::vec2 v2);
	void AddConstant(int location, glm::vec3 v3);
	void AddConstant(int location, glm::vec4 v4);
	void AddConstant(int location, glm::mat3 m3);
	void AddConstant(int location, glm::mat4 m4);
	void BindConstants();

	inline std::vector<std::pair<int, std::pair<Texture, Sampler>>> GetTextureBindings() { return m_uniformT; }
	inline std::vector<std::pair<int, Image>> GetImageBindings() { return m_uniformI; }
	inline std::vector<std::pair<int, int>> GetIntBindings() { return m_uniform1i; }
	inline std::vector<std::pair<int, float>> GetFloatBindings() { return m_uniform1f; }
	inline std::vector<std::pair<int, glm::vec2>> GetVec2Bindings() { return m_uniform2f; }
	inline std::vector<std::pair<int, glm::vec3>> GetVec3Bindings() { return m_uniform3f; }
	inline std::vector<std::pair<int, glm::vec4>> GetVec4Bindings() { return m_uniform4f; }
	inline std::vector<std::pair<int, glm::mat3>> GetMat3Bindings() { return m_uniform3fvMat; }
	inline std::vector<std::pair<int, glm::mat4>> GetMat4Bindings() { return m_uniform4fvMat; }

private:
	std::vector<std::pair<int, std::pair<Texture, Sampler>>> m_uniformT;
	std::vector<std::pair<int, Image>> m_uniformI;
	std::vector<std::pair<int, int>> m_uniform1i;
	std::vector<std::pair<int, float>> m_uniform1f;
	std::vector<std::pair<int, glm::vec2>> m_uniform2f;
	std::vector<std::pair<int, glm::vec3>> m_uniform3f;
	std::vector<std::pair<int, glm::vec4>> m_uniform4f;
	std::vector<std::pair<int, glm::mat3>> m_uniform3fvMat;
	std::vector<std::pair<int, glm::mat4>> m_uniform4fvMat;
};

