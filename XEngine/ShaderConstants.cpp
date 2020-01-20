#include "ShaderConstants.h"

template<class T>
static void FindDuplicateAndReplace(std::vector<std::pair<int, T>>& vec, int location, T data)
{
	int index = 0;
	for (std::pair<int, T> pair : vec)
	{
		if (pair.first == location)
			break;
		++index;
	}
	if (index == vec.size())
		vec.push_back(std::make_pair(location, data));
	else
		vec[index] = std::make_pair(location, data);
}

void ShaderConstants::AddConstant(int location, Texture texture, Sampler sampler)
{
	FindDuplicateAndReplace(m_uniformT, location, std::make_pair(texture, sampler));
}

void ShaderConstants::AddConstant(int location, Image image)
{
	FindDuplicateAndReplace(m_uniformI, location, image);
}

void ShaderConstants::AddConstant(int location, int i)
{
	FindDuplicateAndReplace(m_uniform1i, location, i);
}

void ShaderConstants::AddConstant(int location, float f)
{
	FindDuplicateAndReplace(m_uniform1f, location, f);
}

void ShaderConstants::AddConstant(int location, glm::vec2 v2)
{
	FindDuplicateAndReplace(m_uniform2f, location, v2);
}

void ShaderConstants::AddConstant(int location, glm::vec3 v3)
{
	FindDuplicateAndReplace(m_uniform3f, location, v3);
}

void ShaderConstants::AddConstant(int location, glm::vec4 v4)
{
	FindDuplicateAndReplace(m_uniform4f, location, v4);
}

void ShaderConstants::AddConstant(int location, glm::mat3 m3)
{
	FindDuplicateAndReplace(m_uniform3fvMat, location, m3);
}

void ShaderConstants::AddConstant(int location, glm::mat4 m4)
{
	FindDuplicateAndReplace(m_uniform4fvMat, location, m4);
}

void ShaderConstants::BindConstants()
{
	for (std::pair<int, std::pair<Texture, Sampler>> lts : m_uniformT)
		glUniform1i(lts.first, lts.second.first.BindToSampler(lts.second.second));
	for (std::pair<int, Image> lts : m_uniformI)
		glUniform1i(lts.first, lts.second.BindToUnit());
	for (std::pair<int, int> lts : m_uniform1i)
		glUniform1i(lts.first, lts.second);
	for (std::pair<int, float> lts : m_uniform1f)
		glUniform1f(lts.first, lts.second);
	for (std::pair<int, glm::vec2> lts : m_uniform2f)
		glUniform2f(lts.first, lts.second.x, lts.second.y);
	for (std::pair<int, glm::vec3> lts : m_uniform3f)
		glUniform3f(lts.first, lts.second.x, lts.second.y, lts.second.z);
	for (std::pair<int, glm::vec4> lts : m_uniform4f)
		glUniform4f(lts.first, lts.second.x, lts.second.y, lts.second.z, lts.second.w);
	for (std::pair<int, glm::mat3> lts : m_uniform3fvMat)
		glUniformMatrix3fv(lts.first, 1, false, &lts.second[0][0]);
	for (std::pair<int, glm::mat4> lts : m_uniform4fvMat)
		glUniformMatrix4fv(lts.first, 1, false, &lts.second[0][0]);
}
