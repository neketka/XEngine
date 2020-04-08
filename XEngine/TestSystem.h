#pragma once

class TestSystem : public ISystem
{
public:
	virtual std::string GetName() override;
	virtual std::vector<std::string> GetComponentTypes() override;

	virtual void Update(float deltaTime, ComponentDataIterator& data) override;
};

class TestComponent : public Component
{
public:
	int instance;
	float myValue;
};