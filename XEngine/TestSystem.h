#pragma once
#include "GraphicsDefs.h"
#include "MeshAsset.h"

class TestSystem : public ISystem
{
public:
	virtual void Initialize() override;
	virtual void Destroy() override;
	virtual std::string GetName() override;
	virtual std::vector<std::string> GetComponentTypes() override;
	virtual int GetMaxPostThreadCount() { return 0; }

	virtual void Update(float deltaTime, ComponentDataIterator& data) override;
	virtual void AfterEntityUpdate(float deltaTime) override;
	virtual void PostUpdate(float deltaTime, int threadIndex) override;
private:
	RefCountedAsset<MeshAsset> m_cubeA;
	PinnedGPUMemory m_cubeAVerts;
	PinnedGPUMemory m_cubeAInds;

	GraphicsCommandBuffer *m_cmdTopBuffer;

	GraphicsShader *m_vshader;
	GraphicsShader *m_fshader;
	GraphicsRenderPass *m_renderPass;
	GraphicsRenderTarget *m_renderTarget;
	GraphicsImageObject *m_depthBufferObject;
	GraphicsImageView *m_depthBufferView;
	GraphicsRenderPipeline *m_pipeline;
	GraphicsMemoryBuffer *m_vertexData;

	GraphicsShaderDataSet *m_shaderData;

	GraphicsSpecificShaderCode *m_shaderVertexCode;
	GraphicsSpecificShaderCode *m_shaderFragmentCode;
	GraphicsSpecificSpecializationData *m_specData;
};

class TestComponent : public Component
{
public:
	bool initialized;
	int instance;
	float myValue;
};