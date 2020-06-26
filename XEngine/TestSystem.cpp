#include "pch.h"
#include "TestSystem.h"
#include "GLShaderCompilation.h"
#include "testimage.h"

class TestEntityData
{
public:
	TestComponent *Component;
};

void TestSystem::Initialize()
{
	GraphicsContext *context = XEngine::GetInstance().
		GetInterface<DisplayInterface>(HardwareInterfaceType::Display)->GetGraphicsContext();

	m_shaderVertexCode = new GLShaderCode(R"glsl(
out gl_PerVertex
{
  vec4 gl_Position;
  float gl_PointSize;
  float gl_ClipDistance[];
};

layout(location = 0) out vec2 uvOut;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 uv;

void main()
{
	uvOut = uv;
	gl_Position = vec4(pos, 1);
}

)glsl", ShaderStageBit::Vertex);
	m_shaderFragmentCode = new GLShaderCode(R"glsl(
layout(location = 0) out vec4 color;
layout(location = 0) in vec2 uv;

layout(location = 0) uniform sampler2D tex;

void main()
{
	color = texture(tex, uv);
}

)glsl", ShaderStageBit::Fragment);

	glm::vec3 vertices[] = {
		glm::vec3(0, 0, 0.5f),
		glm::vec3(0, 1, 0.5f),
		glm::vec3(1, 0, 0.5f)
	};

	glm::vec2 uvs[] = {
		glm::vec2(0, 0),
		glm::vec2(0, 1),
		glm::vec2(1, 0)
	};

	m_vshader = context->CreateShader(m_shaderVertexCode);
	m_fshader = context->CreateShader(m_shaderFragmentCode);

	m_specData = new GLShaderSpecialization({});

	GraphicsSamplerState smstate;
	smstate.AnisotropicFiltering = false;
	smstate.DepthComparison = false;
	smstate.EnableMipmapping = false;
	smstate.LODBias = 0;
	smstate.Magnification = SamplerFilteringMode::Nearest;
	smstate.Minification = SamplerFilteringMode::Nearest;
	smstate.WrapU = SamplerWrapMode::ClampToEdge;
	smstate.WrapV = SamplerWrapMode::ClampToEdge;
	smstate.WrapW = SamplerWrapMode::ClampToEdge;
	smstate.MinLOD = -1000;
	smstate.MaxLOD = 1000;
	smstate.MaxAnisotropy = 1;
	smstate.DepthComparisonMode = ComparisonMode::Less;

	m_sampler = context->CreateSampler(smstate);
	m_texture = context->CreateImage(ImageType::Image2D, VectorDataFormat::R8G8B8A8Unorm, glm::ivec3(testwidth, testheight, 1), 1,
		ImageUsageBit::TransferDest | ImageUsageBit::Sampling);
	m_textureView = m_texture->CreateView(ImageType::Image2D, VectorDataFormat::R8G8B8A8Unorm, 0, 1, 0, 1, ImageSwizzleComponent::Red, ImageSwizzleComponent::Green,
		ImageSwizzleComponent::Blue, ImageSwizzleComponent::Alpha);

	GraphicsShaderResourceViewData textureData;
	textureData.Binding = 0;
	textureData.Count = 1;
	textureData.Stages = ShaderStageBit::Fragment;
	textureData.Type = ShaderResourceType::ImageAndSampler;

	m_shaderData = context->CreateShaderDataSet({ textureData }, {});
	m_shaderResourceInstance = context->CreateShaderResourceInstance(textureData);
	context->UpdateShaderImageSamplerResourceInstance(m_shaderResourceInstance, { m_textureView }, { m_sampler });

	GraphicsRenderSubpassState rss;
	rss.ColorAttachments = { 0 };

	GraphicsRenderPassState rps;
	rps.LoadOperations = { GraphicsRenderPassLoadStore::Clear, GraphicsRenderPassLoadStore::Clear };
	rps.StoreOperations = { GraphicsRenderPassLoadStore::Preserve, GraphicsRenderPassLoadStore::Preserve };
	rps.Subpasses = { rss };

	m_renderPass = context->CreateRenderPass(rps);

	m_depthBufferObject = context->CreateImage(ImageType::Image2D, VectorDataFormat::D32Float, glm::ivec3(800, 600, 1), 1, ImageUsageBit::DepthStencil);
	m_depthBufferView = m_depthBufferObject->CreateView(ImageType::Image2D, VectorDataFormat::D32Float, 0, 1, 0, 1, ImageSwizzleComponent::Red,
		ImageSwizzleComponent::Green, ImageSwizzleComponent::Blue, ImageSwizzleComponent::Alpha);
	
	m_renderTarget = context->CreateRenderTarget({ context->GetScreenImageView() }, m_depthBufferView, m_renderPass, 800, 600, 1);

	GraphicsShaderState vss;
	vss.Shader = m_vshader;
	vss.Specialization = m_specData;

	GraphicsShaderState fss;
	fss.Shader = m_fshader;
	fss.Specialization = m_specData;

	GraphicsRenderVertexAttributeData att;
	att.BufferBinding = 0;
	att.Format = VectorDataFormat::R32G32B32Float;
	att.InElementOffset = 0;
	att.ShaderAttributeLocation = 0;

	GraphicsRenderVertexBufferInputData vbid;
	vbid.BufferBinding = 0;
	vbid.Instanced = false;
	vbid.Stride = sizeof(glm::vec3);

	GraphicsRenderVertexAttributeData att1;
	att1.BufferBinding = 1;
	att1.Format = VectorDataFormat::R32G32Float;
	att1.InElementOffset = 0;
	att1.ShaderAttributeLocation = 1;

	GraphicsRenderVertexBufferInputData vbid1;
	vbid1.BufferBinding = 1;
	vbid1.Instanced = false;
	vbid1.Stride = sizeof(glm::vec2);

	GraphicsRenderPipelineState ps;
	ps.BlendingState.DoLogicOperations = false;

	ps.DepthStencilState.EnableState = true;
	ps.DepthStencilState.DepthTest = true;
	ps.DepthStencilState.DepthCompareOp = ComparisonMode::Less;
	ps.DepthStencilState.DepthWrite = true;

	ps.RasterState.EnableState = true;
	ps.RasterState.CullFrontFace = false;
	ps.RasterState.CullBackFace = false;
	ps.RasterState.RasterizerDiscard = false;
	ps.RasterState.ClockwiseFrontFace = false;
	ps.RasterState.DrawingMode = PolygonMode::Fill;
	ps.RasterState.LineWidth = 1;

	ps.RenderTessellationState.EnableState = false;
	ps.Shaders = { vss, fss };
	ps.ShaderDataSet = m_shaderData;

	ps.ViewportState.ViewportBounds = glm::vec4(0, 0, 800, 600);
	ps.ViewportState.ScissorBounds = glm::vec4(0, 0, 800, 600);

	ps.VertexInputState.PrimitiveRestart = false;
	ps.VertexInputState.Topology = GraphicsPrimitiveType::Triangles;
	ps.VertexInputState.AttributeData = { att, att1 };
	ps.VertexInputState.BufferData = { vbid, vbid1 };
	
	m_pipeline = context->CreateGraphicsPipeline(ps);
	
	m_stagingBuffer = context->CreateBuffer(16e6, BufferUsageBit::TransferSource, GraphicsMemoryTypeBit::HostVisible | GraphicsMemoryTypeBit::Coherent);

	m_vertexData = context->CreateBuffer(3 * sizeof(glm::vec3) + 3 * sizeof(glm::vec2), BufferUsageBit::TransferDest | BufferUsageBit::VertexBuffer,
		GraphicsMemoryTypeBit::DeviceResident);
	
	m_stagingBuffer->MapBuffer(0, 16e3, true, true);

	std::memcpy(m_stagingBuffer->GetMappedPointer(), vertices, 3 * sizeof(glm::vec3));
	std::memcpy(reinterpret_cast<char *>(m_stagingBuffer->GetMappedPointer()) + 3 * sizeof(glm::vec3), uvs, 3 * sizeof(glm::vec2));

	GraphicsBufferImageCopyRegion texRegion;
	texRegion.BufferImageHeight = 0;
	texRegion.BufferRowLength = 0;
	texRegion.BufferOffset = 3 * sizeof(glm::vec3) + 3 * sizeof(glm::vec2);
	texRegion.Level = 0;
	texRegion.Offset = glm::ivec3(0, 0, 0);
	texRegion.Size = glm::ivec3(testwidth, testheight, 1);

	for (int i = 0; i < testwidth * testheight; ++i)
	{
		char *location = reinterpret_cast<char *>(m_stagingBuffer->GetMappedPointer()) + 3 * sizeof(glm::vec3) + 3 * sizeof(glm::vec2) + i * 4;
		HEADER_PIXEL(header_data, location);
	}

	m_cmdRenderBuffer = context->CreateGraphicsCommandBuffers(1, false, false, false)[0];
	m_cmdRenderBuffer->BeginRecording();
	m_cmdRenderBuffer->CopyBufferToBuffer(m_stagingBuffer, m_vertexData, 0, 0, 3 * sizeof(glm::vec3) + 3 * sizeof(glm::vec2));
	m_cmdRenderBuffer->CopyBufferToImageWithConversion(m_stagingBuffer, VectorDataFormat::R8G8B8A8Uint, m_texture, { texRegion });
	m_cmdRenderBuffer->StopRecording();

	context->SubmitCommands(m_cmdRenderBuffer, GraphicsQueueType::Transfer);

	m_cmdTopBuffer = context->CreateGraphicsCommandBuffers(1, false, false, false)[0];
	m_cmdTopBuffer->BeginRecording();
	m_cmdTopBuffer->BindRenderPass(m_renderTarget, m_renderPass, { glm::vec4(0, 0, 0, 1), glm::vec4(1, 1, 1, 1) }, 0);
	m_cmdTopBuffer->BindRenderPipeline(m_pipeline);
	m_cmdTopBuffer->BindVertexBuffers(0, { m_vertexData, m_vertexData }, { 0, 3 * sizeof(glm::vec3) });
	m_cmdTopBuffer->BindRenderShaderResourceInstance(m_shaderData, m_shaderResourceInstance, 0, 0);
	m_cmdTopBuffer->Draw(3, 1, 0, 0);
	m_cmdTopBuffer->EndRenderPass();
	m_cmdTopBuffer->StopRecording();
}

void TestSystem::Destroy()
{
	GraphicsContext *context = XEngine::GetInstance().
		GetInterface<DisplayInterface>(HardwareInterfaceType::Display)->GetGraphicsContext();
	context->SyncWithCommandSubmissionThread();

	m_stagingBuffer->UnmapBuffer();

	delete m_textureView;
	delete m_texture;
	delete m_shaderResourceInstance;
	delete m_sampler;
	delete m_textureStagingBuffer;
	delete m_cmdTopBuffer;
	delete m_vshader;
	delete m_fshader;
	delete m_shaderData;
	delete m_renderPass;
	delete m_depthBufferView;
	delete m_depthBufferObject;
	delete m_renderTarget;
	delete m_pipeline;
	delete m_cmdRenderBuffer;
	delete m_vertexData;
	delete m_stagingBuffer;
	delete m_shaderVertexCode;
	delete m_shaderFragmentCode;
	delete m_specData;
}

std::string TestSystem::GetName()
{
	return "TestSystem";
}

std::vector<std::string> TestSystem::GetComponentTypes()
{
	return { "TestComponent" };
}

int instance = 0;
void TestSystem::Update(float deltaTime, ComponentDataIterator& data)
{
	TestEntityData *dataPointer;
	while (dataPointer = data.Next<TestEntityData>())
	{
		if (!dataPointer->Component->initialized)
		{
			dataPointer->Component->initialized = true;
			dataPointer->Component->myValue = 0;
			dataPointer->Component->instance = instance++;
		}
		for (int i = 0; i < 100; ++i)
		{
			dataPointer->Component->myValue += deltaTime * 2;
		}
	}
}

void TestSystem::AfterEntityUpdate(float deltaTime)
{
	XEngine::GetInstance().GetInterface<DisplayInterface>(HardwareInterfaceType::Display)->GetGraphicsContext()
		->SubmitCommands(m_cmdTopBuffer, GraphicsQueueType::Graphics);
}

void TestSystem::PostUpdate(float deltaTime, int threadIndex)
{
}
