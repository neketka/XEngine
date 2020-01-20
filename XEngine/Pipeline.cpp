#include "Pipeline.h"

void Pipeline::Optimize(Pipeline& previous)
{ 
	// Minimize state changes
	if (Shader.GetID() == previous.Shader.GetID())
		m_keepShader = true;

	if (this->VertexStage.VertexArray.GetID() == previous.VertexStage.VertexArray.GetID())
		this->VertexStage._keepAttributes = true;
	
	if (this->TransformFeedbackStage.BufferBindings == previous.TransformFeedbackStage.BufferBindings)
		this->TransformFeedbackStage._keepBindings = true;

	if (this->FragmentStage.Framebuffer.GetID() == previous.FragmentStage.Framebuffer.GetID())
		this->FragmentStage._keepFBO = true;

	if (this->FragmentStage.DrawTargets == previous.FragmentStage.DrawTargets)
		this->FragmentStage._keepTargets = true;
}

void Pipeline::BindPipeline()
{
	if (!this->m_keepShader)
		glUseProgram(this->Shader.GetID());
	//Vertex Stage
	if (!this->VertexStage._keepAttributes)
		glBindVertexArray(this->VertexStage.VertexArray.GetID());

	if (this->VertexStage.FrontFaceCulling || this->VertexStage.BackFaceCulling)
	{
		glEnable(GL_CULL_FACE);
		if (this->VertexStage.FrontFaceCulling && this->VertexStage.BackFaceCulling)
			glCullFace(GL_FRONT_AND_BACK);
		else if (this->VertexStage.FrontFaceCulling)
			glCullFace(GL_FRONT);
		else if (this->VertexStage.BackFaceCulling)
			glCullFace(GL_BACK);
	}
	else
		glDisable(GL_CULL_FACE);

	if (this->VertexStage.DepthClamping)
		glEnable(GL_DEPTH_CLAMP);
	else
		glDisable(GL_DEPTH_CLAMP);

	//Tesselation Stage
	glPatchParameteri(GL_PATCH_VERTICES, this->TessellationStage.PatchSize);

	//Transform Feedback Stage
	if (!this->TransformFeedbackStage._keepBindings)
		for (std::pair<int, TBuffer> buffers : this->TransformFeedbackStage.BufferBindings)
			glBindBufferBase(GL_TRANSFORM_FEEDBACK, buffers.first, buffers.second.GetID());

	//Fragment Stage
	if (!this->FragmentStage._keepFBO)
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->FragmentStage.Framebuffer.GetID());


	if (this->FragmentStage.ScissorTest)
	{
		glEnable(GL_SCISSOR_TEST);
		glScissor(static_cast<GLint>(this->FragmentStage.ScissorRegion.Position.x), static_cast<GLint>(this->FragmentStage.ScissorRegion.Position.y),
			static_cast<GLsizei>(this->FragmentStage.ScissorRegion.Size.x), static_cast<GLsizei>(this->FragmentStage.ScissorRegion.Size.y));
	}
	else
		glDisable(GL_SCISSOR_TEST);

	if (this->FragmentStage.RasterizerDiscard) glEnable(GL_RASTERIZER_DISCARD);
	else glDisable(GL_RASTERIZER_DISCARD);

	if (this->FragmentStage.DepthTest) glEnable(GL_DEPTH_TEST);
	else glDisable(GL_DEPTH_TEST);

	if (this->FragmentStage.StencilTest) glEnable(GL_STENCIL_TEST);
	else glDisable(GL_STENCIL_TEST);

	glDepthMask(this->FragmentStage.DepthMask);
	glStencilMaskSeparate(GL_FRONT, this->FragmentStage.FrontStencilMask);
	glStencilMaskSeparate(GL_BACK, this->FragmentStage.BackStencilMask);
	for (ColorMask mask : this->FragmentStage.ColorMasks)
		glColorMaski(mask.Buffer, mask.Red, mask.Green, mask.Blue, mask.Alpha);
	/*
	glStencilFuncSeparate(static_cast<GLenum>(this->FragmentStage.FrontStencilFunc), static_cast<GLenum>(this->FragmentStage.BackStencilFunc),
		this->FragmentStage.StencilCompareReference, this->FragmentStage.StencilCompareMask);

	glStencilOpSeparate(GL_FRONT, static_cast<GLenum>(this->FragmentStage.FrontStencilFail), static_cast<GLenum>(this->FragmentStage.FrontDepthFail), 
		static_cast<GLenum>(this->FragmentStage.FrontDepthPass));

	glStencilOpSeparate(GL_BACK, static_cast<GLenum>(this->FragmentStage.BackStencilFail), static_cast<GLenum>(this->FragmentStage.BackDepthFail),
		static_cast<GLenum>(this->FragmentStage.BackDepthPass));*/

	if (this->FragmentStage.Blending)
	{
		glEnable(GL_BLEND);
		for (BufferBlending blending : this->FragmentStage.BufferBlends)
		{
			glBlendFuncSeparatei(blending.Buffer, static_cast<GLenum>(blending.SourceColor), static_cast<GLenum>(blending.DestinationColor),
				static_cast<GLenum>(blending.SourceAlpha), static_cast<GLenum>(blending.DestinationAlpha));
			glBlendEquationSeparatei(blending.Buffer, static_cast<GLenum>(blending.ColorEquation), static_cast<GLenum>(blending.AlphaEquation));
		}
	}
	else
		glDisable(GL_BLEND);
	
	glViewport(static_cast<GLint>(this->FragmentStage.Viewport.Position.x), static_cast<GLint>(this->FragmentStage.Viewport.Position.y),
			static_cast<GLsizei>(this->FragmentStage.Viewport.Size.x), static_cast<GLsizei>(this->FragmentStage.Viewport.Size.y));

	if (!this->FragmentStage._keepTargets && this->FragmentStage.Framebuffer.GetID() != 0)
	{
		std::vector<GLenum> drawTargets;
		for (int target : this->FragmentStage.DrawTargets)
			drawTargets.push_back(GL_COLOR_ATTACHMENT0 + target);
		glDrawBuffers(static_cast<GLsizei>(this->FragmentStage.DrawTargets.size()), drawTargets.data());
	}
}
