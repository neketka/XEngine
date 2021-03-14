#include "pch.h"

#include "GLCmdBuffer.h"

#include "GLContext.h"
#include "GLPipeline.h"
#include "GLShaderData.h"
#include "GLBuffer.h"
#include "GLImage.h"
#include "GLQuery.h"

std::map<ComparisonMode, GLenum> compModeB =
{
	{ ComparisonMode::Never, GL_NEVER }, { ComparisonMode::Less, GL_LESS }, { ComparisonMode::Equal, GL_EQUAL },
	{ ComparisonMode::LessOrEqual, GL_LEQUAL }, { ComparisonMode::Greater, GL_GREATER }, { ComparisonMode::NotEqual, GL_NOTEQUAL },
	{ ComparisonMode::GreaterOrEqual, GL_GEQUAL }, { ComparisonMode::Always, GL_ALWAYS }
};

GLCmdBuffer::GLCmdBuffer(GraphicsContext *context, bool pooled) : m_context(context), m_pooled(pooled)
{
}

GLCmdBuffer::~GLCmdBuffer()
{
	for (void *buffer : m_buffersHeld)
	{
		std::free(buffer);
	}
}

void GLCmdBuffer::BindRenderPipeline(GraphicsRenderPipeline *pipeline)
{
	m_lastPipeCompute = false;
	GLPipeline *p = m_lastRenderPipe = dynamic_cast<GLPipeline *>(pipeline);

	m_frontCompMask = p->GetState().DepthStencilState.StencilFront.CompareMask;
	m_frontRefMask = p->GetState().DepthStencilState.StencilFront.Reference;
	m_backCompMask = p->GetState().DepthStencilState.StencilBack.CompareMask;
	m_backRefMask = p->GetState().DepthStencilState.StencilBack.Reference;
	m_commands.push_back([p]()
		{
			p->BindState();
		});

	m_minTextureUnit = 0;
	m_minImageUnit = 0;
}

void GLCmdBuffer::BindComputePipeline(GraphicsComputePipeline *pipeline)
{
	m_lastPipeCompute = true;
	GLComputePipeline *p = m_lastComputePipe = dynamic_cast<GLComputePipeline *>(pipeline);
	m_commands.push_back([p]()
		{
			p->BindState();
		});

	m_minTextureUnit = 0;
	m_minImageUnit = 0;
}

void GLCmdBuffer::BindRenderPass(GraphicsRenderTarget *target, GraphicsRenderPass *renderPass, std::vector<glm::vec4>&& attachmentClearValues, char stencil)
{
	GLRenderTarget *rt = m_lastRenderTarget = dynamic_cast<GLRenderTarget *>(target);
	GLRenderPass *rp = m_lastPass = dynamic_cast<GLRenderPass *>(renderPass);
	m_subpassIndexCounter = 0;

	m_commands.push_back([attachmentClearValues, rt, rp, stencil]()
		{
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, rt->GetFBOId());
		 	
			rp->PerformPreOps(const_cast<std::vector<glm::vec4> &>(attachmentClearValues), stencil);
			rp->BindSubpass(0);

		});
}

void GLCmdBuffer::NextSubpass()
{
	GLRenderPass *rp = m_lastPass;
	int32_t subpass = ++m_subpassIndexCounter;
	m_commands.push_back([rp, subpass]()
		{
			rp->BindSubpass(subpass);
		});
}

void GLCmdBuffer::EndRenderPass()
{
	GLRenderPass *rp = m_lastPass;
	m_commands.push_back([rp]()
		{
			rp->PerformPostOps();
		});
}

void GLCmdBuffer::BeginQuery(GraphicsQuery *query)
{
}

void GLCmdBuffer::EndQuery(GraphicsQuery *query)
{
}

void GLCmdBuffer::ResetQuery(GraphicsQuery *query)
{
}

void GLCmdBuffer::WriteTimestamp(GraphicsQuery *query)
{
}

void GLCmdBuffer::WriteQueryToBuffer(GraphicsQuery *query, int32_t bufferOffset)
{
}

void GLCmdBuffer::BindVertexBuffers(int32_t firstBinding, std::vector<GraphicsMemoryBuffer *> buffers, std::vector<int32_t> offsets)
{
	std::vector<GLuint> ids(buffers.size());
	std::vector<GLsizei> strides(buffers.size());
	std::vector<GLintptr> offs(offsets.size());

	GLPipeline *p = m_lastRenderPipe;

	for (int32_t i = 0; i < buffers.size(); ++i)
	{
		ids[i] = dynamic_cast<GLBuffer *>(buffers[i])->GetBufferId();
		strides[i] = p->GetBindingStride(firstBinding + i);
		offs[i] = offsets[i];
	}

	m_commands.push_back([firstBinding, ids, offs, strides, p]()
		{
			glVertexArrayVertexBuffers(p->GetVao(), firstBinding, offs.size(), ids.data(), offs.data(), strides.data());
		});
}

void GLCmdBuffer::BindIndexBuffer(GraphicsMemoryBuffer *buffer, bool dataType16bit)
{
	GLuint id = dynamic_cast<GLBuffer *>(buffer)->GetBufferId();
	GLPipeline *p = m_lastRenderPipe;
	m_commands.push_back([id, p]()
		{
			glVertexArrayElementBuffer(p->GetVao(), id);
		});
	m_ibo16Bit = dataType16bit;
}

void GLCmdBuffer::PushShaderConstants(GraphicsShaderDataSet *set, int32_t constantIndex, int32_t constantOffset, int32_t constantCount, void *data)
{
	m_lastRenderPipe->WaitInit();
	GLShaderDataSet *sds = dynamic_cast<GLShaderDataSet *>(set);
	GraphicsShaderConstantData& sdata = sds->GetConstantData()[constantIndex];

	std::vector<GLuint> progs;
	progs.reserve(4);
	if (sdata.Stages & ShaderStageBit::Compute)
		progs.push_back(m_lastComputePipe->GetComputeProgram());
	if (sdata.Stages & ShaderStageBit::Fragment)
		progs.push_back(m_lastRenderPipe->GetStage(ShaderStageBit::Fragment));
	if (sdata.Stages & ShaderStageBit::Geometry)
		progs.push_back(m_lastRenderPipe->GetStage(ShaderStageBit::Geometry));
	if (sdata.Stages & ShaderStageBit::TessControl)
		progs.push_back(m_lastRenderPipe->GetStage(ShaderStageBit::TessControl));
	if (sdata.Stages & ShaderStageBit::TessEval)
		progs.push_back(m_lastRenderPipe->GetStage(ShaderStageBit::TessEval));
	if (sdata.Stages & ShaderStageBit::Vertex)
		progs.push_back(m_lastRenderPipe->GetStage(ShaderStageBit::Vertex));

	void *cmdBufStore = std::malloc(4 * sdata.ElementCount);
	std::memcpy(cmdBufStore, data, 4 * sdata.ElementCount);
	m_buffersHeld.push_back(cmdBufStore);

	if (sdata.Float)
	{
		if (sdata.Matrix)
		{
			decltype(glProgramUniformMatrix2fv) func = sdata.VectorLength == 4 ? glProgramUniformMatrix2fv : sdata.VectorLength == 9 ? 
				glProgramUniformMatrix3fv : glProgramUniformMatrix4fv;
			for (GLuint prog : progs)
				m_commands.push_back(std::bind(func, prog, sdata.Offset + constantOffset, constantCount, false, reinterpret_cast<float *>(cmdBufStore)));
		}
		else
		{
			decltype(glProgramUniform1fv) func = sdata.VectorLength == 1 ? glProgramUniform1fv : sdata.VectorLength == 2 ? 
				glProgramUniform2fv : sdata.VectorLength == 3 ? glProgramUniform3fv : glProgramUniform4fv;
			for (GLuint prog : progs)
				m_commands.push_back(std::bind(func, prog, sdata.Offset + constantOffset, constantCount, reinterpret_cast<float *>(cmdBufStore)));
		}
	}
	else
	{
		if (sdata.Unsigned)
		{
			decltype(glProgramUniform1uiv) func = sdata.VectorLength == 1 ? glProgramUniform1uiv : sdata.VectorLength == 2 ?
				glProgramUniform2uiv : sdata.VectorLength == 3 ? glProgramUniform3uiv : glProgramUniform4uiv;
			for (GLuint prog : progs)
				m_commands.push_back(std::bind(func, prog, sdata.Offset + constantOffset, constantCount, reinterpret_cast<uint32_t *>(cmdBufStore)));
		}
		else
		{
			decltype(glProgramUniform1iv) func = sdata.VectorLength == 1 ? glProgramUniform1iv : sdata.VectorLength == 2 ?
				glProgramUniform2iv : sdata.VectorLength == 3 ? glProgramUniform3iv : glProgramUniform4iv;
			for (GLuint prog : progs)
				m_commands.push_back(std::bind(func, prog, sdata.Offset + constantOffset, constantCount, reinterpret_cast<int32_t *>(cmdBufStore)));
		}
	}
}

void GLCmdBuffer::BindRenderShaderResourceInstance(GraphicsShaderDataSet *set, GraphicsShaderResourceInstance *instance, int32_t viewIndex, int32_t offset)
{
	if (m_lastRenderPipe)
		m_lastRenderPipe->WaitInit();
	GLShaderDataSet *ds = dynamic_cast<GLShaderDataSet *>(set);
	GLShaderResourceInstance *ri = dynamic_cast<GLShaderResourceInstance *>(instance);

	GraphicsShaderResourceViewData& data = ds->GetResourceViews()[viewIndex];

	std::vector<GLuint> progs;
	progs.reserve(4);
	if (data.Stages & ShaderStageBit::Compute)
		progs.push_back(m_lastComputePipe->GetComputeProgram());
	if (data.Stages & ShaderStageBit::Fragment)
		progs.push_back(m_lastRenderPipe->GetStage(ShaderStageBit::Fragment));
	if (data.Stages & ShaderStageBit::Geometry)
		progs.push_back(m_lastRenderPipe->GetStage(ShaderStageBit::Geometry));
	if (data.Stages & ShaderStageBit::TessControl)
		progs.push_back(m_lastRenderPipe->GetStage(ShaderStageBit::TessControl));
	if (data.Stages & ShaderStageBit::TessEval)
		progs.push_back(m_lastRenderPipe->GetStage(ShaderStageBit::TessEval));
	if (data.Stages & ShaderStageBit::Vertex)
		progs.push_back(m_lastRenderPipe->GetStage(ShaderStageBit::Vertex));

	int32_t minTUnit = m_minTextureUnit;
	int32_t minIUnit = m_minImageUnit;

	switch (data.Type)
	{
	case ShaderResourceType::ImageAndSampler:
	case ShaderResourceType::InputAttachment:
		m_minTextureUnit += data.Count;
		for (GLuint prog : progs)
		{
			m_commands.push_back([data, prog, ri, minTUnit]() {
				glBindSamplers(minTUnit, data.Count, ri->GetSamplerIds().data());
				glBindTextures(minTUnit, data.Count, ri->GetViewIds().data());
				for (int32_t i = 0; i < data.Count; ++i)
					glProgramUniform1i(prog, data.Binding + i, i + minTUnit);
				});
		}
		break;
	case ShaderResourceType::ImageLoadStore:
		for (GLuint prog : progs)
		{
			minIUnit += data.Count;
			m_commands.push_back([data, prog, ri, minIUnit]() {
				glBindImageTextures(minIUnit, data.Count, ri->GetViewIds().data());
				for (int32_t i = 0; i < data.Count; ++i)
					glProgramUniform1i(prog, data.Binding + i, i + minIUnit);
				});
		}
		break;
	case ShaderResourceType::StorageBuffer:
		m_commands.push_back([data, ri, offset]() {
			glBindBufferRange(GL_SHADER_STORAGE_BUFFER, data.Binding, dynamic_cast<GLBuffer *>(ri->GetBuffer())->GetBufferId(), 
				ri->GetBufferOffset(), ri->GetBufferRange());
			});
		break;
	case ShaderResourceType::StorageBufferDynamic:
		m_commands.push_back([data, ri, offset]() {
			glBindBufferRange(GL_SHADER_STORAGE_BUFFER, data.Binding, dynamic_cast<GLBuffer *>(ri->GetBuffer())->GetBufferId(),
				ri->GetBufferOffset() + offset, ri->GetBufferRange());
			});
		break;
	case ShaderResourceType::UniformBuffer:
		m_commands.push_back([data, ri, offset]() {
			glBindBufferRange(GL_UNIFORM_BUFFER, data.Binding, dynamic_cast<GLBuffer *>(ri->GetBuffer())->GetBufferId(),
				ri->GetBufferOffset(), ri->GetBufferRange());
			});
		break;
	case ShaderResourceType::UniformBufferDynamic:
		m_commands.push_back([data, ri, offset]() {
			glBindBufferRange(GL_UNIFORM_BUFFER, data.Binding, dynamic_cast<GLBuffer *>(ri->GetBuffer())->GetBufferId(),
				ri->GetBufferOffset() + offset, ri->GetBufferRange());
			});
		break;
	}
}

void GLCmdBuffer::BindComputeShaderResourceInstance(GraphicsShaderDataSet *set, GraphicsShaderResourceInstance *instance, int32_t viewIndex, int32_t offset)
{
	m_lastComputePipe->WaitInit();
	BindRenderShaderResourceInstance(set, instance, viewIndex, offset);
}

void GLCmdBuffer::DrawIndexed(int32_t vertexCount, int32_t instances, int32_t firstIndex, int32_t vertexOffset, int32_t firstInstance)
{
	GLenum indexType = m_ibo16Bit ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
	GLenum topology = m_lastRenderPipe->GetTopology() == GraphicsPrimitiveType::Triangles ? GL_TRIANGLES :
		m_lastRenderPipe->GetTopology() == GraphicsPrimitiveType::Lines ? GL_LINES :
		m_lastRenderPipe->GetTopology() == GraphicsPrimitiveType::Patches ? GL_PATCHES : GL_POINTS;
	
	m_commands.push_back(std::bind(glDrawElementsInstancedBaseVertexBaseInstance, topology, vertexCount, indexType, reinterpret_cast<void *>(firstIndex), instances, vertexOffset, firstInstance));
}

void GLCmdBuffer::Draw(int32_t vertexCount, int32_t instanceCount, int32_t firstVertex, int32_t firstInstance)
{
	GLenum topology = m_lastRenderPipe->GetTopology() == GraphicsPrimitiveType::Triangles ? GL_TRIANGLES :
		m_lastRenderPipe->GetTopology() == GraphicsPrimitiveType::Lines ? GL_LINES :
		m_lastRenderPipe->GetTopology() == GraphicsPrimitiveType::Patches ? GL_PATCHES : GL_POINTS;
	m_commands.push_back(std::bind(glDrawArraysInstancedBaseInstance, topology, firstVertex, vertexCount, instanceCount, firstInstance));
}

void GLCmdBuffer::DrawIndirect(GraphicsMemoryBuffer *buffer, int32_t offset, int32_t drawCount, int32_t stride)
{
	GLenum topology = m_lastRenderPipe->GetTopology() == GraphicsPrimitiveType::Triangles ? GL_TRIANGLES :
		m_lastRenderPipe->GetTopology() == GraphicsPrimitiveType::Lines ? GL_LINES :
		m_lastRenderPipe->GetTopology() == GraphicsPrimitiveType::Patches ? GL_PATCHES : GL_POINTS;
	m_commands.push_back([buffer, topology, offset, drawCount, stride]() {
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, dynamic_cast<GLBuffer *>(buffer)->GetBufferId());
		glMultiDrawArraysIndirect(topology, reinterpret_cast<void *>(offset), drawCount, stride);
		});
}

void GLCmdBuffer::DrawIndirectIndexed(GraphicsMemoryBuffer *buffer, int32_t offset, int32_t drawCount, int32_t stride)
{
	GLenum indexType = m_ibo16Bit ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
	GLenum topology = m_lastRenderPipe->GetTopology() == GraphicsPrimitiveType::Triangles ? GL_TRIANGLES :
		m_lastRenderPipe->GetTopology() == GraphicsPrimitiveType::Lines ? GL_LINES :
		m_lastRenderPipe->GetTopology() == GraphicsPrimitiveType::Patches ? GL_PATCHES : GL_POINTS;
	m_commands.push_back([buffer, topology, offset, drawCount, stride, indexType]() {

		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, dynamic_cast<GLBuffer *>(buffer)->GetBufferId());
		glMultiDrawElementsIndirect(topology, indexType, reinterpret_cast<void *>(offset), drawCount, stride);
		});
}

void GLCmdBuffer::DrawIndirectCount(GraphicsMemoryBuffer *buffer, int32_t offset, GraphicsMemoryBuffer *drawCountBuffer, int32_t drawCountBufferOffset, int32_t maxDrawCount, int32_t stride)
{
	GLenum topology = m_lastRenderPipe->GetTopology() == GraphicsPrimitiveType::Triangles ? GL_TRIANGLES :
		m_lastRenderPipe->GetTopology() == GraphicsPrimitiveType::Lines ? GL_LINES :
		m_lastRenderPipe->GetTopology() == GraphicsPrimitiveType::Patches ? GL_PATCHES : GL_POINTS;
	m_commands.push_back([buffer, topology, offset, drawCountBufferOffset, maxDrawCount, stride, drawCountBuffer]() {

		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, dynamic_cast<GLBuffer *>(buffer)->GetBufferId());
		glBindBuffer(GL_PARAMETER_BUFFER_ARB, dynamic_cast<GLBuffer *>(drawCountBuffer)->GetBufferId());
		glMultiDrawArraysIndirectCountARB(topology, reinterpret_cast<void *>(offset), drawCountBufferOffset, maxDrawCount, stride);
		});
}

void GLCmdBuffer::DrawIndirectIndexedCount(GraphicsMemoryBuffer *buffer, int32_t offset, GraphicsMemoryBuffer *drawCountBuffer, int32_t drawCountBufferOffset, int32_t maxDrawCount, int32_t stride)
{
	GLenum indexType = m_ibo16Bit ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
	GLenum topology = m_lastRenderPipe->GetTopology() == GraphicsPrimitiveType::Triangles ? GL_TRIANGLES :
		m_lastRenderPipe->GetTopology() == GraphicsPrimitiveType::Lines ? GL_LINES :
		m_lastRenderPipe->GetTopology() == GraphicsPrimitiveType::Patches ? GL_PATCHES : GL_POINTS;
	m_commands.push_back([buffer, topology, offset, drawCountBufferOffset, maxDrawCount, stride, drawCountBuffer, indexType]() {
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, dynamic_cast<GLBuffer *>(buffer)->GetBufferId());
		glBindBuffer(GL_PARAMETER_BUFFER_ARB, dynamic_cast<GLBuffer *>(drawCountBuffer)->GetBufferId());
		glMultiDrawElementsIndirectCountARB(topology, indexType, reinterpret_cast<void *>(offset), drawCountBufferOffset, maxDrawCount, stride);
		});
}

void GLCmdBuffer::DispatchCompute(int32_t x, int32_t y, int32_t z)
{
	m_commands.push_back(std::bind(glDispatchCompute, x, y, z));
}

void GLCmdBuffer::DispatchIndirect(GraphicsMemoryBuffer *buffer, int32_t offset)
{
	m_commands.push_back([buffer, offset]() {
		glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, dynamic_cast<GLBuffer *>(buffer)->GetBufferId());
		glDispatchComputeIndirect(offset);
		});
}

void GLCmdBuffer::UpdatePipelineDynamicViewport(glm::vec4 dimensions)
{
	m_commands.push_back(std::bind(glViewport, dimensions.x, dimensions.y, dimensions.z, dimensions.w));
}

void GLCmdBuffer::UpdatePipelineDynamicScissorBox(glm::vec4 dimensions)
{
	m_commands.push_back(std::bind(glScissor, dimensions.x, dimensions.y, dimensions.z, dimensions.w));
}

void GLCmdBuffer::UpdatePipelineDynamicLineWidth(float lineWidth)
{
	m_commands.push_back(std::bind(glLineWidth, lineWidth));
}

void GLCmdBuffer::UpdatePipelineDynamicStencilFrontFaceCompareMask(uint32_t mask)
{
	GLenum func = compModeB[m_lastRenderPipe->GetState().DepthStencilState.StencilFront.CompareOperation];
	m_frontCompMask = mask;

	m_commands.push_back(std::bind(glStencilFuncSeparate, GL_FRONT, func, m_frontRefMask, mask));
}

void GLCmdBuffer::UpdatePipelineDynamicStencilFrontFaceWriteMask(uint32_t mask)
{
	m_commands.push_back(std::bind(glStencilMaskSeparate, GL_FRONT, mask));
}

void GLCmdBuffer::UpdatePipelineDynamicStencilFrontFaceReference(uint32_t mask)
{
	GLenum func = compModeB[m_lastRenderPipe->GetState().DepthStencilState.StencilFront.CompareOperation];
	m_frontRefMask = mask;

	m_commands.push_back(std::bind(glStencilFuncSeparate, GL_FRONT, func, mask, m_frontCompMask));
}

void GLCmdBuffer::UpdatePipelineDynamicStencilBackFaceCompareMask(uint32_t mask)
{
	GLenum func = compModeB[m_lastRenderPipe->GetState().DepthStencilState.StencilBack.CompareOperation];
	m_backCompMask = mask;

	m_commands.push_back(std::bind(glStencilFuncSeparate, GL_BACK, func, m_backRefMask, mask));
}

void GLCmdBuffer::UpdatePipelineDynamicStencilBackFaceWriteMask(uint32_t mask)
{
	m_commands.push_back(std::bind(glStencilMaskSeparate, GL_BACK, mask));
}

void GLCmdBuffer::UpdatePipelineDynamicStencilBackFaceReference(uint32_t mask)
{
	GLenum func = compModeB[m_lastRenderPipe->GetState().DepthStencilState.StencilBack.CompareOperation];
	m_backRefMask = mask;

	m_commands.push_back(std::bind(glStencilFuncSeparate, GL_BACK, func, mask, m_backCompMask));
}

void GLCmdBuffer::UpdatePipelineDynamicBlendConstant(glm::vec4 constants)
{
	m_commands.push_back(std::bind(glBlendColor, constants.r, constants.g, constants.b, constants.a));
}

void GLCmdBuffer::WaitOnFence(GraphicsSyncObject *sync)
{
	GLContext *c = dynamic_cast<GLContext *>(m_context);
	m_commands.push_back([sync, c]() {
		glWaitSync(dynamic_cast<GLSync *>(sync)->GetSync(), 0, GL_TIMEOUT_IGNORED);
		});
}

void GLCmdBuffer::SignalFence(GraphicsSyncObject *sync)
{
	GLContext *c = dynamic_cast<GLContext *>(m_context);
	m_commands.push_back([sync, c]() {
		GLsync s = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
		dynamic_cast<GLSync *>(sync)->SetSync(s);
		c->WaitForSync(sync);
		glFlush();
		});
}

void GLCmdBuffer::ResetFence(GraphicsSyncObject *sync)
{
	m_commands.push_back([sync]() {
		dynamic_cast<GLSync *>(sync)->ResetSignaled();
		});
}

void GLCmdBuffer::SynchronizeMemory(MemoryBarrierBit from, MemoryBarrierBit to, bool byRegion)
{
	GLbitfield bits = 0;
	if (byRegion)
	{
		if (to & MemoryBarrierBit::RenderTargetRead)
			bits |= GL_FRAMEBUFFER_BARRIER_BIT;
		if (to & MemoryBarrierBit::ShaderReadBit)
			bits |= GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_QUERY_BUFFER_BARRIER_BIT;
		if (to & MemoryBarrierBit::UniformBufferRead)
			bits |= GL_UNIFORM_BARRIER_BIT;
		if (to & MemoryBarrierBit::InputAttachmentRead)
			bits |= GL_SHADER_IMAGE_ACCESS_BARRIER_BIT;
		m_commands.push_back([bits]() {
			glMemoryBarrierByRegion(bits);
			});
	}
	else
	{
		if (to & MemoryBarrierBit::VertexAttribRead)
			bits |= GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT;
		if (to & MemoryBarrierBit::UniformBufferRead)
			bits |= GL_UNIFORM_BARRIER_BIT;
		if (to & MemoryBarrierBit::TransferRead)
			bits |= GL_PIXEL_BUFFER_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT;
		if (to & MemoryBarrierBit::ShaderReadBit)
			bits |= GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_QUERY_BUFFER_BARRIER_BIT;
		if (to & MemoryBarrierBit::RenderTargetRead)
			bits |= GL_FRAMEBUFFER_BARRIER_BIT;
		if (to & MemoryBarrierBit::InputAttachmentRead)
			bits |= GL_TEXTURE_UPDATE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT;
		if (to & MemoryBarrierBit::IndirectCommandRead)
			bits |= GL_COMMAND_BARRIER_BIT;
		if (to & MemoryBarrierBit::IndexBufferRead)
			bits |= GL_ELEMENT_ARRAY_BARRIER_BIT;
		if (to & MemoryBarrierBit::HostRead)
			bits |= GL_PIXEL_BUFFER_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT | GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT;
		m_commands.push_back([bits]() {
			glMemoryBarrier(bits);
			});
	}
}

void GLCmdBuffer::ClearColor(GraphicsImageObject *image, glm::vec4 color, int32_t level, int32_t layer)
{
	GLImage *i = dynamic_cast<GLImage *>(image);
	VectorFormatDesc desc = GetFormatDesc(i->GetVectorFormat());
	int32_t dataSize = (desc.RBits > 0 ? 1 : 0) + (desc.GBits > 0 ? 1 : 0) + (desc.BBits > 0 ? 1 : 0) + (desc.ABits > 0 ? 1 : 0);
	GLenum format = dataSize == 1 ? GL_R : dataSize == 2 ? GL_RG : dataSize == 3 ? GL_RGB : GL_RGBA;
	int32_t depth = layer == -1 ? i->GetSize().z : 1;
	m_commands.push_back([i, color, level, layer, dataSize, format, depth]() {
		glClearTexSubImage(i->GetImageId(), level, 0, 0, layer, i->GetSize().x, i->GetSize().y, depth, format, GL_FLOAT, &color.r);
		});
}

void GLCmdBuffer::ClearDepthStencil(GraphicsImageObject *image, float depth, char stencil)
{
	GLImage *i = dynamic_cast<GLImage *>(image);
	VectorFormatDesc desc = GetFormatDesc(i->GetVectorFormat());
	if (i->GetFormat() == GL_STENCIL_INDEX8)
	{
		m_commands.push_back([i, stencil]() {
			glClearTexSubImage(i->GetImageId(), 0, 0, 0, 0, i->GetSize().x, i->GetSize().y, 1, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, &stencil);
			});
	}
	else if (desc.GBits > 0)
	{
		uint32_t f[] = { *reinterpret_cast<uint32_t *>(&depth), stencil >> 24 };
		m_commands.push_back([i, f]() {
			glClearTexSubImage(i->GetImageId(), 0, 0, 0, 0, i->GetSize().x, i->GetSize().y, 1, GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV, f);
			});
	}
	else if (desc.RBits > 0)
	{
		m_commands.push_back([i, depth]() {
			glClearTexSubImage(i->GetImageId(), 0, 0, 0, 0, i->GetSize().x, i->GetSize().y, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
			});
	}
}

void GLCmdBuffer::ClearAttachmentsColor(int32_t index, glm::vec4 color)
{
	m_commands.push_back([index, color]() {
		glClearBufferfv(GL_COLOR, index, &color.x);
		});
}

void GLCmdBuffer::ClearAttachmentsDepthStencil(int32_t index, float depth, char stencil)
{
	m_commands.push_back([index, depth, stencil]() {
		glClearBufferfi(GL_DEPTH_STENCIL, 0, depth, stencil);
		});
}

void GLCmdBuffer::UpdateBufferData(GraphicsMemoryBuffer *buffer, int32_t offset, int32_t size, void *data)
{
	void *copied = std::malloc(size);
	std::memcpy(copied, data, size);
	m_buffersHeld.push_back(copied);

	m_commands.push_back([buffer, offset, size, copied]()
		{
			glNamedBufferSubData(dynamic_cast<GLBuffer *>(buffer)->GetBufferId(), offset, size, copied);
		});
}

void GLCmdBuffer::CopyBufferToImageWithConversion(GraphicsMemoryBuffer *srcBuffer, VectorDataFormat srcFormat, GraphicsImageObject *destImage, std::vector<GraphicsBufferImageCopyRegion> regions)
{
	GLBuffer *src = dynamic_cast<GLBuffer *>(srcBuffer);
	GLImage *dest = dynamic_cast<GLImage *>(destImage);
	VectorFormatDesc desc = GetFormatDesc(srcFormat);

	GLenum format = desc.Depth ? (desc.Stencil ? GL_DEPTH_STENCIL : GL_DEPTH_COMPONENT) : desc.Stencil ? GL_STENCIL_INDEX :
		desc.RBits > 0 ? (desc.GBits > 0 ? (desc.BBits > 0 ? (desc.ABits > 0 ? GL_RGBA : GL_RGB) : GL_RG) : GL_R) : GL_R;
	GLenum type = (!desc.Depth || !desc.Stencil) ? ((desc.Integer || desc.Normalized) ? (desc.Unsigned ? (desc.RBits == 8 ? GL_UNSIGNED_BYTE : (desc.RBits == 16 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT)) :
		desc.RBits == 8 ? GL_BYTE : (desc.RBits == 16 ? GL_SHORT : GL_INT)) : GL_FLOAT) : (desc.RBits == 24 ? GL_UNSIGNED_INT_24_8 : GL_FLOAT_32_UNSIGNED_INT_24_8_REV);

	switch (dest->GetTarget())
	{
	case GL_TEXTURE_1D:
		m_commands.push_back([src, dest, regions, format, type]()
			{
				glBindBuffer(GL_PIXEL_UNPACK_BUFFER, src->GetBufferId());
				for (const GraphicsBufferImageCopyRegion& reg : regions)
				{
					glPixelStorei(GL_UNPACK_ROW_LENGTH, reg.BufferRowLength);
					glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, reg.BufferImageHeight);
					glTextureSubImage1D(dest->GetImageId(), reg.Level, reg.Offset.x, reg.Size.x, format, type, reinterpret_cast<void *>(reg.BufferOffset));
				}
			});
		break;
	case GL_TEXTURE_2D:
	case GL_TEXTURE_1D_ARRAY:
	case GL_TEXTURE_CUBE_MAP:
		m_commands.push_back([src, dest, regions, format, type]()
			{
				glBindBuffer(GL_PIXEL_UNPACK_BUFFER, src->GetBufferId());
				for (const GraphicsBufferImageCopyRegion& reg : regions)
				{
					glPixelStorei(GL_UNPACK_ROW_LENGTH, reg.BufferRowLength);
					glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, reg.BufferImageHeight);
					glTextureSubImage2D(dest->GetImageId(), reg.Level, reg.Offset.x, reg.Offset.y, reg.Size.x,
						reg.Size.y, format, type, reinterpret_cast<void *>(reg.BufferOffset));
				}
			});
		break;
	case GL_TEXTURE_3D:
	case GL_TEXTURE_2D_ARRAY:
	case GL_TEXTURE_CUBE_MAP_ARRAY:
		m_commands.push_back([src, dest, regions, format, type]()
			{
				glBindBuffer(GL_PIXEL_UNPACK_BUFFER, src->GetBufferId());
				for (const GraphicsBufferImageCopyRegion& reg : regions)
				{
					glPixelStorei(GL_UNPACK_ROW_LENGTH, reg.BufferRowLength);
					glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, reg.BufferImageHeight);
					glTextureSubImage3D(dest->GetImageId(), reg.Level, reg.Offset.x, reg.Offset.y, reg.Offset.z, reg.Size.x,
						reg.Size.y, reg.Size.z, format, type, reinterpret_cast<void *>(reg.BufferOffset));
				}
			});
		break;
	}
}

void GLCmdBuffer::CopyImageToBuffer(GraphicsImageObject *srcImage, GraphicsMemoryBuffer *destBuffer, std::vector<GraphicsBufferImageCopyRegion> regions)
{
	GLImage *dest = dynamic_cast<GLImage *>(destBuffer);
	GLBuffer *src = dynamic_cast<GLBuffer *>(srcImage);
	VectorFormatDesc desc = GetFormatDesc(dest->GetVectorFormat());

	GLenum format = desc.Depth ? (desc.Stencil ? GL_DEPTH_STENCIL : GL_DEPTH_COMPONENT) : desc.Stencil ? GL_STENCIL_INDEX :
		desc.RBits > 0 ? (desc.GBits > 0 ? (desc.BBits > 0 ? (desc.ABits > 0 ? GL_RGBA : GL_RGB) : GL_RG) : GL_R) : GL_R;
	GLenum type = (desc.Depth && desc.Stencil) ? (desc.Integer ? (desc.Unsigned ? (desc.RBits == 8 ? GL_UNSIGNED_BYTE : (desc.RBits == 16 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT)) :
		desc.RBits == 8 ? GL_BYTE : (desc.RBits == 16 ? GL_SHORT : GL_INT)) : GL_FLOAT) : (desc.RBits == 24 ? GL_UNSIGNED_INT_24_8 : GL_FLOAT_32_UNSIGNED_INT_24_8_REV);

	int32_t size = (desc.RBits + desc.GBits + desc.BBits + desc.ABits) / 4;

	m_commands.push_back([src, dest, regions, format, type]()
		{
			glBindBuffer(GL_PIXEL_PACK_BUFFER, src->GetBufferId());
			for (const GraphicsBufferImageCopyRegion& reg : regions)
			{
				glPixelStorei(GL_PACK_ROW_LENGTH, reg.BufferRowLength);
				glPixelStorei(GL_PACK_IMAGE_HEIGHT, reg.BufferImageHeight);
				glGetTextureSubImage(dest->GetImageId(), reg.Level, reg.Offset.x, reg.Offset.y, reg.Offset.z, reg.Size.x,
					reg.Size.y, reg.Size.z, format, type, src->GetSize(), reinterpret_cast<void *>(reg.BufferOffset));
			}
		});
}

void GLCmdBuffer::CopyImageToImage(GraphicsImageObject *srcImage, GraphicsImageObject *destImage, glm::ivec3 srcOffset, glm::ivec3 destOffset, glm::ivec3 size, 
	int32_t srcLevel, int32_t destLevel, int32_t layers, bool color, bool depth, bool stencil)
{
	GLImage *src = dynamic_cast<GLImage *>(srcImage);
	GLImage *dest = dynamic_cast<GLImage *>(destImage); 
	m_commands.push_back([src, dest, srcOffset, destOffset, size, srcLevel, destLevel, layers]()
		{
			glCopyImageSubData(src->GetImageId(), src->GetTarget(), srcLevel, srcOffset.x, srcOffset.y, srcOffset.z, dest->GetImageId(), 
				dest->GetTarget(), destLevel, destOffset.x, destOffset.y, destOffset.z, size.x, size.y, size.z);
		});

}

void GLCmdBuffer::CopyBufferToBuffer(GraphicsMemoryBuffer *src, GraphicsMemoryBuffer *dest, uint64_t srcOffset, uint64_t destOffset, int32_t size)
{
	GLBuffer *s = dynamic_cast<GLBuffer *>(src);
	GLBuffer *d = dynamic_cast<GLBuffer *>(dest);
	m_commands.push_back([s, d, srcOffset, destOffset, size]()
		{
			glCopyNamedBufferSubData(s->GetBufferId(), d->GetBufferId(), srcOffset, destOffset, size);
		});
}

void GLCmdBuffer::BeginRecording()
{
	for (void *buffer : m_buffersHeld)
	{
		std::free(buffer);
	}
	m_buffersHeld.clear();
	m_commands.clear();
}

void GLCmdBuffer::StopRecording()
{
}

void GLCmdBuffer::ExecuteSubCommandBuffer(GraphicsCommandBuffer *commandBuffer)
{
	m_commands.push_back(std::bind(&GLCmdBuffer::Execute, dynamic_cast<GLCmdBuffer *>(commandBuffer)));
}

void GLCmdBuffer::Execute()
{
	for (std::function<void()>& f : m_commands)
		f();
}