#include "Framebuffer.h"

Framebuffer::Framebuffer()
{
}

Framebuffer::Framebuffer(std::vector<WrappedTextureView> colorAttachments, WrappedTextureView depthStencil)
{
	glCreateFramebuffers(1, &m_id);
	int index = 0;
	for (WrappedTextureView colorAttachment : colorAttachments)
	{
		if (colorAttachment.GetLayer() == -1)
			glNamedFramebufferTexture(m_id, GL_COLOR_ATTACHMENT0 + index, colorAttachment.GetID(),
				colorAttachment.GetLevel());
		else
			glNamedFramebufferTextureLayer(m_id, GL_COLOR_ATTACHMENT0 + index, colorAttachment.GetID(),
			colorAttachment.GetLevel(), colorAttachment.GetLayer());
		++index;
	}
	if (depthStencil.GetLayer() == -1)
		glNamedFramebufferTexture(m_id, GL_DEPTH_STENCIL_ATTACHMENT, depthStencil.GetID(), depthStencil.GetLevel());
	else
		glNamedFramebufferTextureLayer(m_id, GL_DEPTH_STENCIL_ATTACHMENT, depthStencil.GetID(), depthStencil.GetLevel(), depthStencil.GetLayer());
}

Framebuffer::Framebuffer(std::vector<WrappedTextureView> colorAttachments, WrappedTextureView depth, WrappedTextureView stencil, bool hasDepth, bool hasStencil)
{
	glCreateFramebuffers(1, &m_id);
	int index = 0;
	for (WrappedTextureView colorAttachment : colorAttachments)
	{
		if (colorAttachment.GetLayer() == -1)
			glNamedFramebufferTexture(m_id, GL_COLOR_ATTACHMENT0 + index, colorAttachment.GetID(),
				colorAttachment.GetLevel());
		else
			glNamedFramebufferTextureLayer(m_id, GL_COLOR_ATTACHMENT0 + index, colorAttachment.GetID(),
				colorAttachment.GetLevel(), colorAttachment.GetLayer());
		++index;
	}
	if (hasDepth)
	{
		if (depth.GetLayer() == -1)
			glNamedFramebufferTexture(m_id, GL_DEPTH_ATTACHMENT, depth.GetID(), depth.GetLevel());
		else
			glNamedFramebufferTextureLayer(m_id, GL_DEPTH_ATTACHMENT, depth.GetID(), depth.GetLevel(), depth.GetLayer());
	}
	if (hasStencil)
	{
		if (stencil.GetLayer() == -1)
			glNamedFramebufferTexture(m_id, GL_STENCIL_ATTACHMENT, stencil.GetID(), stencil.GetLevel());
		else
			glNamedFramebufferTextureLayer(m_id, GL_STENCIL_ATTACHMENT, stencil.GetID(), stencil.GetLevel(), stencil.GetLayer());
	}
}

void Framebuffer::ClearColor(int drawBuffer, glm::vec4 color)
{
	if (m_id == 0)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT);
	}
	else
		glClearNamedFramebufferfv(m_id, GL_COLOR, drawBuffer, &color[0]);
}

void Framebuffer::ClearDepth(float value)
{
	glDepthMask(true);
	glClearNamedFramebufferfv(m_id, GL_DEPTH, 0, &value);
}

void Framebuffer::Destroy()
{
	if (m_id != 0)
		glDeleteFramebuffers(1, &m_id);
	m_id = 0;
}
