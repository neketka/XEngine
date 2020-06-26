#include "pch.h"
#include "XEngine.h"
#include "GLContext.h"
#include "GLPipeline.h"
#include "GLShaderData.h"
#include "GLBuffer.h"
#include "GLImage.h"
#include "GLQuery.h"

DisposableHandle::~DisposableHandle() {}
GraphicsImageObject::~GraphicsImageObject() {}
GraphicsMemoryBuffer::~GraphicsMemoryBuffer() {}
GraphicsSyncObject::~GraphicsSyncObject() {}

GLContext::GLContext(SDL_Window *window)
{
	m_window = window;
	m_running = true;
	m_specific = new GLSpecific;
	m_initScreen = false;
	m_syncWithRenderThread = false;

	glm::ivec2 size;
	SDL_GL_GetDrawableSize(static_cast<SDL_Window *>(window), &size.x, &size.y);
	m_atomicScreenSize = size;

	m_glSubmissionThread = new std::thread(&GLContext::RunContextThread, this);
	while (!m_initScreen);
}

GLContext::~GLContext()
{
	m_running = false;

	delete m_colorImage;
	delete m_colorView;
	delete m_renderTarget;

	m_glSubmissionThread->join();

	delete m_specific;
	delete m_glSubmissionThread;
}

std::vector<GraphicsCommandBuffer *> GLContext::CreateGraphicsCommandBuffers(int count, bool graphics, bool compute, bool transfer)
{
	std::vector<GraphicsCommandBuffer *> buffers;
	buffers.reserve(count);
	for (int i = 0; i < count; ++i)
	{
		buffers.push_back(new GLCmdBuffer(this));
	}
	return buffers;
}

GraphicsRenderPipeline *GLContext::CreateGraphicsPipeline(GraphicsRenderPipelineState& state)
{
	GLPipeline *p = new GLPipeline(this, state);
	m_queuedInitializers.push(p);
	return p;
}

GraphicsComputePipeline *GLContext::CreateComputePipeline(GraphicsComputePipelineState& state)
{
	GLComputePipeline *p = new GLComputePipeline(this, state);
	m_queuedInitializers.push(p);
	return p;
}

GraphicsMemoryBuffer *GLContext::CreateBuffer(unsigned long long byteSize, BufferUsageBit usage, GraphicsMemoryTypeBit mem)
{
	GLBuffer *b = new GLBuffer(this, byteSize, mem);
	m_queuedInitializers.push(b);
	return b;
}

GraphicsImageObject *GLContext::CreateImage(ImageType type, VectorDataFormat format, glm::ivec3 size, int miplevels, ImageUsageBit usage)
{
	GLImage *i = new GLImage(this, format, size, miplevels, usage, type);
	m_queuedInitializers.push(i);
	return i;
}

GraphicsRenderTarget *GLContext::CreateRenderTarget(std::vector<GraphicsImageView *>&& attachments, GraphicsImageView *depthStencil, GraphicsRenderPass *renderPass, int width, int height, int layers)
{
	GLRenderTarget *t = new GLRenderTarget(this, std::forward<std::vector<GraphicsImageView *>>(attachments), depthStencil, width, height, layers);
	m_queuedInitializers.push(t);
	return t;
}

GraphicsShaderDataSet *GLContext::CreateShaderDataSet(std::vector<GraphicsShaderResourceViewData>&& resourceViews, std::vector<GraphicsShaderConstantData>&& constantData)
{
	return new GLShaderDataSet(std::forward<std::vector<GraphicsShaderResourceViewData>>(resourceViews), std::forward<std::vector<GraphicsShaderConstantData>>(constantData));
}

GraphicsShaderResourceInstance *GLContext::CreateShaderResourceInstance(GraphicsShaderResourceViewData& data)
{
	GLShaderResourceInstance *ri = new GLShaderResourceInstance(data);
	m_queuedInitializers.push(ri);
	return ri;
}

GraphicsSampler *GLContext::CreateSampler(GraphicsSamplerState& state)
{
	GLSampler *s = new GLSampler(this, state);
	m_queuedInitializers.push(s);
	return s;
}

GraphicsShader *GLContext::CreateShader(GraphicsSpecificShaderCode *code)
{
	GLShader *s = new GLShader(this, dynamic_cast<GLShaderCode *>(code));
	return s;
}

std::vector<GraphicsQuery *> GLContext::CreateQueries(int count, GraphicsQueryType type)
{
	std::vector<GraphicsQuery *> queries;
	queries.reserve(count);
	for (int i = 0; i < count; ++i)
	{
		GLQuery *q = new GLQuery(this, type);
		m_queuedInitializers.push(q);
		queries.push_back(q);
	}
	return queries; 
}

GraphicsImageView *GLContext::GetScreenImageView()
{
	return m_colorView;
}

glm::ivec2 GLContext::GetScreenSize()
{
	return m_atomicScreenSize;
}

GraphicsSpecificStructure& GLContext::GetSpecificStructure()
{
	return *m_specific;
}

void GLContext::UpdateShaderImageSamplerResourceInstance(GraphicsShaderResourceInstance *instance, std::vector<GraphicsImageView *>&& imageViews, std::vector<GraphicsSampler *>&& samplers)
{
	GLShaderResourceInstance *inst = dynamic_cast<GLShaderResourceInstance *>(instance);
	inst->BindImageSamplers(imageViews, samplers);
}

void GLContext::UpdateShaderImageInputAttachmentInstance(GraphicsShaderResourceInstance *instance, std::vector<GraphicsImageView *>&& imageViews)
{
	GLShaderResourceInstance *inst = dynamic_cast<GLShaderResourceInstance *>(instance);
	inst->BindImages(imageViews);
}

void GLContext::UpdateShaderImageLoadStoreResourceInstance(GraphicsShaderResourceInstance *instance, std::vector<GraphicsImageView *>&& imageViews)
{
	GLShaderResourceInstance *inst = dynamic_cast<GLShaderResourceInstance *>(instance);
	inst->BindImages(imageViews);
}

void GLContext::UpdateShaderBufferResourceInstance(GraphicsShaderResourceInstance *instance, GraphicsMemoryBuffer *buffer, int offset, int range)
{
	GLShaderResourceInstance *inst = dynamic_cast<GLShaderResourceInstance *>(instance);
	inst->BindBuffer(buffer, offset, range);
}

void GLContext::SubmitCommands(GraphicsCommandBuffer *commands, GraphicsQueueType queue)
{
	m_queuedBuffers.push(dynamic_cast<GLCmdBuffer *>(commands));
}

void GLContext::Present()
{
	m_queuedBuffers.push(nullptr);
	++m_framesInProgress;
}

void GLContext::ResizeScreen(glm::ivec2 size)
{
	m_atomicScreenSize = size;
}

void GLContext::DeleteInitable(std::function<void()> initable)
{
	m_queuedDeleters.push(initable);
}

void GLContext::EnqueueInitable(GLInitable *initable)
{
	m_queuedInitializers.push(initable);
}

void GLContext::WaitUntilFramesFinishIfEqualTo(int bufferedFrames)
{
	if (m_framesInProgress >= bufferedFrames)
		while (m_framesInProgress != 0);
}

void GLContext::WaitForSync(GraphicsSyncObject *sync)
{
	m_queuedSyncs.push(sync);
}

void GLContext::MapRequest(GLBuffer *buffer)
{
	m_queuedMaps.push(buffer);
	while (!buffer->GetMapRequest().Helper);
}

void GLAPIENTRY
MessageCallback(GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam)
{
	fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
		(type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
		type, severity, message);
}

void GLContext::RunContextThread()
{
	m_context = SDL_GL_CreateContext(m_window);
	SDL_GL_MakeCurrent(m_window, m_context);

	glewInit();

	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(MessageCallback, 0);

	glm::ivec2 size = m_atomicScreenSize;
	m_colorImage = CreateImage(ImageType::Image2D, VectorDataFormat::R8G8B8A8Unorm, glm::ivec3(size, 1), 1, ImageUsageBit::Color);
	m_colorView = m_colorImage->CreateView(ImageType::Image2D, VectorDataFormat::R8G8B8A8Unorm, 0, 1, 0, 1, ImageSwizzleComponent::Red,
		ImageSwizzleComponent::Green, ImageSwizzleComponent::Blue, ImageSwizzleComponent::Alpha);
	m_renderTarget = CreateRenderTarget({ m_colorView }, nullptr, nullptr, size.x, size.y, 1);

	m_initScreen = true;

	GLInitable *dest;
	GLCmdBuffer *buffer;
	GraphicsSyncObject *sync;
	GLBuffer *map;

	int statusInt = 0;

	std::function<void()> deleter;
	std::vector<GraphicsSyncObject *> pushSyncs;
	glm::ivec2 windowSize = GetScreenSize();

	while (m_running)
	{
		m_syncWithRenderThread = false;

		glm::ivec2 frameWindowSize = GetScreenSize();
		if (frameWindowSize != windowSize)
		{
			windowSize = frameWindowSize;
			delete m_colorImage;
			delete m_colorView;
			delete m_renderTarget;

			m_colorImage = CreateImage(ImageType::Image2D, VectorDataFormat::R8G8B8A8Unorm, glm::ivec3(windowSize, 1), 1, ImageUsageBit::Color);
			m_colorView = m_colorImage->CreateView(ImageType::Image2D, VectorDataFormat::R8G8B8A8Unorm, 0, 1, 0, 1, ImageSwizzleComponent::Red,
				ImageSwizzleComponent::Green, ImageSwizzleComponent::Blue, ImageSwizzleComponent::Alpha);
			m_renderTarget = CreateRenderTarget({ m_colorView }, nullptr, nullptr, windowSize.x, windowSize.y, 1);
		}

		while (m_queuedInitializers.try_pop(dest))
			dest->InitializeFromContext();
		while (m_queuedMaps.try_pop(map))
		{
			GLBufferMapRequest& req = map->GetMapRequest();
			if (req.RequestMap)
			{
				req.Mapped = glMapNamedBufferRange(map->GetBufferId(), req.Offset, req.Length, req.Bits);
				req.Helper = true;
			}
			else if (req.RequestUnmap)
			{
				glUnmapNamedBuffer(map->GetBufferId());
				req.Helper = true;
			}
			else if (req.RequestInvalidate)
			{
				glInvalidateBufferSubData(map->GetBufferId(), req.Offset, req.Length);
				req.Helper = true;
			}
			else if (req.RequestFlush)
			{
				glFlushMappedNamedBufferRange(map->GetBufferId(), req.Offset, req.Length);
				req.Helper = true;
			}
		}
		while (m_queuedBuffers.try_pop(buffer))
		{
			if (!buffer)
			{
				if (m_running)
				{
					GLRenderTarget *t = dynamic_cast<GLRenderTarget *>(m_renderTarget);
					if (t)
						glBlitNamedFramebuffer(t->GetFBOId(), 0, 0, 0, t->GetWidth(), t->GetHeight(), 0, 0, t->GetWidth(), t->GetHeight(), GL_COLOR_BUFFER_BIT, GL_NEAREST);
				}

				SDL_GL_SwapWindow(m_window);
				--m_framesInProgress;
				break;
			}
			else
			{
				buffer->Execute();
			}
		}
		while (m_queuedSyncs.try_pop(sync))
		{
			GLSync *s = dynamic_cast<GLSync *>(sync);
			if (s->IsWaiting())
			{
				GLenum e = glClientWaitSync(s->GetSync(), GL_SYNC_FLUSH_COMMANDS_BIT, s->GetWaitTime());
				s->Signal((e == GL_ALREADY_SIGNALED || e == GL_CONDITION_SATISFIED) && !(e == GL_TIMEOUT_EXPIRED || e == GL_WAIT_FAILED));
			}
			else
			{
				glGetSynciv(s->GetSync(), GL_SYNC_STATUS, sizeof(int), nullptr, &statusInt);
				if (statusInt == GL_SIGNALED)
					s->Signal(true);
				else
					pushSyncs.push_back(s);
			}
		}
		for (GraphicsSyncObject *s : pushSyncs)
			m_queuedSyncs.push(s);
		pushSyncs.clear();
		while (m_queuedDeleters.try_pop(deleter))
			deleter();

		m_syncWithRenderThread = true;
	}
	SDL_GL_DeleteContext(m_context);
}

void GLContext::SyncWithCommandSubmissionThread()
{
	while (!m_syncWithRenderThread);
}

GraphicsRenderPass *GLContext::CreateRenderPass(GraphicsRenderPassState& state)
{
	return new GLRenderPass(state);
}

GraphicsSyncObject *GLContext::CreateSync(bool gpuQueueSync)
{
	return new GLSync(this);
}

std::string GLSpecific::GetApiString()
{
	return "OpenGL";
}
