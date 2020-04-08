#include "pch.h"
#include "TestSystem.h"

class TestEntityData
{
public:
	TestComponent *Component;
};

std::string TestSystem::GetName()
{
	return "TestSystem";
}

std::vector<std::string> TestSystem::GetComponentTypes()
{
	return { "TestComponent" };
}
int instance = 0;
void TestSystem::Update(float deltaTime, ComponentDataIterator& data)
{
	TestEntityData *dataPointer;
	while (dataPointer = data.Next<TestEntityData>())
	{
		if (!dataPointer->Component->Initialized)
		{
			dataPointer->Component->Initialized = true;
			dataPointer->Component->myValue = 0;
			dataPointer->Component->instance = instance++;
		}
		dataPointer->Component->myValue += deltaTime * 2;
	}
}