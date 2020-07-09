#pragma once

#include "GraphicsDefs.h"
#include "GLInitable.h"
#include <GL/glew.h>
#include <map>

class GLPipeline : public GraphicsRenderPipeline, public GLInitable 
{
public:
	GLPipeline(GraphicsContext *context, GraphicsRenderPipelineState& state);
	virtual ~GLPipeline() override;
	void BindState();
	int GetBindingStride(int binding) { return m_bindingStrides[binding]; }
	GLuint GetStage(ShaderStageBit bits) { return m_stageToProgram[bits]; }
	GLuint GetVao() { return m_vao; }
	GraphicsPrimitiveType GetTopology() { return m_state.VertexInputState.Topology; }
	GraphicsRenderPipelineState& GetState() { return m_state; }
private:
	GraphicsContext *m_context;
	std::map<int, int> m_bindingStrides;
	std::map<ShaderStageBit, GLuint> m_stageToProgram;
	GraphicsRenderPipelineState m_state;
	GLuint m_vao;
	GLuint m_pipeline;

	// Inherited via GLInitable
	virtual void InitInternal() override;
};

class GLComputePipeline : public GraphicsComputePipeline, public GLInitable
{
public:
	GLComputePipeline(GraphicsContext *context, GraphicsComputePipelineState& state);
	virtual ~GLComputePipeline();
	GLuint GetComputeProgram() { return m_program; }
	void BindState();
private:
	GraphicsContext *m_context;
	GraphicsComputePipelineState m_state;
	GLuint m_program;
	GLuint m_pipeline;

	// Inherited via GLInitable
	virtual void InitInternal() override;
};