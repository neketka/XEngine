#include "pch.h"
#include "TestSystem.h"
#include "testimage.h"
#include <glm/gtc/matrix_transform.hpp>

#include <shaderc/shaderc.hpp>

const char *vShader = R"glsl(

#version 450 core

void main()
{
	gl_Position = mvp * vec4(posr.xyz, 1);
}
)glsl";

const char *fShader = R"glsl(

#version 450 core

void main()
{
	float nDotl = dot(normal, -normalize(vec3(-0.5, -0.5, -1)));
	color = vec4(texture(tex, uv).xyz * nDotl, 1);
}
)glsl";

class TestEntityData
{
public:
	TestComponent *Component;
};

void TestSystem::Initialize()
{
}

void TestSystem::Destroy()
{
}

std::string TestSystem::GetName()
{
	return "TestSystem";
}

std::vector<std::string> TestSystem::GetComponentTypes()
{
	return { "TestComponent" };
}

int32_t instance = 0;
void TestSystem::Update(float deltaTime, ComponentDataIterator& data)
{
	TestEntityData *dataPointer;
	while (dataPointer = data.Next<TestEntityData>())
	{
		if (!dataPointer->Component->initialized)
		{
			dataPointer->Component->initialized = true;
			dataPointer->Component->myValue = 0;
			dataPointer->Component->instance = instance++;
		}
		for (int32_t i = 0; i < 100; ++i)
		{
			dataPointer->Component->myValue += deltaTime * 2;
		}
	}
}

float dT = 0;
void TestSystem::AfterEntityUpdate(float deltaTime)
{
}

void TestSystem::PostUpdate(float deltaTime, int32_t threadIndex)
{
}
