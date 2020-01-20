#pragma once
#include <GL\glew.h>
#include <glm/glm.hpp>

#include "GLResource.h"
#include "Texture.h"

class Framebuffer : public IGLResource
{
public:
	Framebuffer();
	Framebuffer(std::vector<WrappedTextureView> colorAttachments, WrappedTextureView depthStencil);
	Framebuffer(std::vector<WrappedTextureView> colorAttachments, WrappedTextureView depth, WrappedTextureView stencil, bool hasDepth, bool hasStencil);
	void ClearColor(int drawBuffer, glm::vec4 color);
	void ClearDepth(float value);
	virtual void Destroy() override;
	GLuint GetID() { return m_id; }
	static Framebuffer GetDefault() { return Framebuffer(0); }
private:
	Framebuffer(GLuint id) : m_id(id) {}
	GLuint m_id;
};

