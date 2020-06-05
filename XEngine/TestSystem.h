#pragma once
#include "GraphicsDefs.h"

class TestSystem : public ISystem
{
public:
	virtual void Initialize() override;
	virtual void Destroy() override;
	virtual std::string GetName() override;
	virtual std::vector<std::string> GetComponentTypes() override;

	virtual void Update(float deltaTime, ComponentDataIterator& data) override;
private:
	GraphicsCommandBuffer *m_cmdTopBuffer;

	GraphicsShader *m_vshader;
	GraphicsShader *m_fshader;
	GraphicsShaderDataSet *m_shaderData;
	GraphicsShaderResourceInstance *m_shaderResourceInstance;
	GraphicsRenderPass *m_renderPass;
	GraphicsRenderTarget *m_renderTarget;
	GraphicsImageObject *m_depthBufferObject;
	GraphicsImageView *m_depthBufferView;
	GraphicsRenderPipeline *m_pipeline;
	GraphicsCommandBuffer *m_cmdRenderBuffer;
	GraphicsMemoryBuffer *m_vertexData;
	
	GraphicsImageObject *m_texture;
	GraphicsImageView *m_textureView;
	GraphicsSampler *m_sampler;
	GraphicsMemoryBuffer *m_textureStagingBuffer;

	GraphicsMemoryBuffer *m_stagingBuffer;
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