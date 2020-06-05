#pragma once

#include "GraphicsDefs.h"
#include "GLInitable.h"
#include "GLShaderCompilation.h"
#include <GL/glew.h>
#include <set>
#include <map>

GLenum ConvertFromShaderStageBit(ShaderStageBit bits);
GLenum ConvertFromShaderStage(ShaderStageBit stage);

class GLShaderDataSet : public GraphicsShaderDataSet
{
public:
	GLShaderDataSet(std::vector<GraphicsShaderResourceViewData>&& resourceViews, std::vector<GraphicsShaderConstantData>&& constantData) 
		: m_resourceViews(resourceViews), m_constantData(constantData) {}
	virtual ~GLShaderDataSet() override {}

	std::vector<GraphicsShaderResourceViewData>& GetResourceViews() { return m_resourceViews; }
	std::vector<GraphicsShaderConstantData>& GetConstantData() { return m_constantData; }
private:
	std::vector<GraphicsShaderResourceViewData> m_resourceViews; 
	std::vector<GraphicsShaderConstantData> m_constantData;
};

class GLShaderResourceInstance : public GraphicsShaderResourceInstance, public GLInitable
{
public:
	GLShaderResourceInstance(GraphicsShaderResourceViewData& data);
	virtual ~GLShaderResourceInstance() override {}

	void BindImages(std::vector<GraphicsImageView *> views) { m_views = views; }
	void BindImageSamplers(std::vector<GraphicsImageView *> views, std::vector<GraphicsSampler *> samplers) { m_views = views; m_samplers = samplers; }
	void BindBuffer(GraphicsMemoryBuffer *buffer, int offset, int range) { m_buffer = buffer; m_offset = offset; m_range = range; }

	int GetBufferOffset() { return m_offset; }
	int GetBufferRange() { return m_range; }
	std::vector<GraphicsImageView *>& GetImageViews() { return m_views; }
	std::vector<GraphicsSampler *>& GetSamplers() { return m_samplers; }
	GraphicsMemoryBuffer *GetBuffer() { return m_buffer; }
	std::vector<GLuint>& GetSamplerIds() { return m_samplerIds; }
	std::vector<GLuint>& GetViewIds() { return m_viewIds; }
private:
	int m_offset;
	int m_range;
	std::vector<GLuint> m_samplerIds;
	std::vector<GLuint> m_viewIds;
	std::vector<GraphicsImageView *> m_views;
	std::vector<GraphicsSampler *> m_samplers;
	GraphicsMemoryBuffer *m_buffer;
	GraphicsShaderResourceViewData m_data;

	virtual void InitInternal() override;
};

class GLShader : public GraphicsShader
{
public:
	GLShader(GraphicsContext *context, GLShaderCode *code) : m_code(code->GetCode(), code->GetStage()), m_context(context) {}
	GLShaderCode& GetShaderCode() { return m_code; }
	GLuint GetSpecialization(GLShaderSpecialization *specialization);

	virtual ~GLShader() override;
private:
	GraphicsContext *m_context;
	std::map<std::set<std::string>, GLuint> m_specializations;
	GLShaderCode m_code;
};