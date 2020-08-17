#pragma once
class TestSystem : public ISystem
{
public:
	virtual void Initialize() override;
	virtual void Destroy() override;
	virtual std::string GetName() override;
	virtual std::vector<std::string> GetComponentTypes() override;
	virtual int32_t GetMaxPostThreadCount() { return 0; }

	virtual void Update(float deltaTime, ComponentDataIterator& data) override;
	virtual void AfterEntityUpdate(float deltaTime) override;
	virtual void PostUpdate(float deltaTime, int32_t threadIndex) override;
private:
};

class TestComponent : public Component
{
public:
	bool initialized;
	int32_t instance;
	float myValue;
};