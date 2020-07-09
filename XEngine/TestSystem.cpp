#include "pch.h"
#include "TestSystem.h"
#include "GLShaderCompilation.h"
#include "testimage.h"
#include <glm/gtc/matrix_transform.hpp>

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

layout(location = 0) smooth out vec3 normal;

layout(location = 0) in vec4 posr;
layout(location = 1) in vec4 normalg;
layout(location = 2) in vec4 tangentb;
layout(location = 3) in vec4 uv0a;

layout(location = 0) uniform mat4 mvp;
layout(location = 1) uniform mat3 nrm;

void main()
{
	normal = nrm * normalg.xyz;
	gl_Position = mvp * vec4(posr.xyz, 1);
}

)glsl", ShaderStageBit::Vertex);
	m_shaderFragmentCode = new GLShaderCode(R"glsl(
layout(location = 0) out vec4 color;
layout(location = 0) smooth in vec3 normal;

void main()
{
	float nDotl = dot(normal, -normalize(vec3(-0.5, -0.5, -1)));
	color = vec4(vec3(0.3) + vec3(1, 1, 1) * nDotl, 1);
}

)glsl", ShaderStageBit::Fragment);

	XEngineInstance->GetAssetManager()->PreloadAssetBundle("./Assets/models.xasset");

	UniqueId cubeAId = XEngineInstance->GetAssetManager()->GetIdByPath("assets/car");
	m_cubeA = XEngineInstance->GetAssetManager()->GetAsset<MeshAsset>(cubeAId);
	m_cubeA.Get().LoadFullMesh();

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

	ps.ViewportState.ViewportBounds = glm::vec4(0, 0, 800, 600);
	ps.ViewportState.ScissorBounds = glm::vec4(0, 0, 800, 600);

	GraphicsShaderConstantData mvp;
	mvp.Float = true;
	mvp.Matrix = true;
	mvp.Offset = 0;
	mvp.VectorLength = 16;
	mvp.Stages = ShaderStageBit::Vertex;
	mvp.ElementCount = 16;

	GraphicsShaderConstantData nrm;
	nrm.Float = true;
	nrm.Matrix = true;
	nrm.Offset = 1;
	nrm.VectorLength = 9;
	nrm.Stages = ShaderStageBit::Vertex;
	nrm.ElementCount = 9;

	m_shaderData = context->CreateShaderDataSet({}, { mvp, nrm });
	ps.ShaderDataSet = m_shaderData;

	MeshAssetLoader *mLoader = static_cast<MeshAssetLoader *>(XEngineInstance->GetAssetManager()->GetLoader("Mesh"));

	mLoader->SetVertexInputState(ps.VertexInputState, m_cubeA.Get().GetVertexFormatId());
	m_pipeline = context->CreateGraphicsPipeline(ps);
}

void TestSystem::Destroy()
{
	GraphicsContext *context = XEngine::GetInstance().
		GetInterface<DisplayInterface>(HardwareInterfaceType::Display)->GetGraphicsContext();
	context->SyncWithCommandSubmissionThread();

	delete m_shaderData;
	delete m_vshader;
	delete m_fshader;
	delete m_renderPass;
	delete m_depthBufferView;
	delete m_depthBufferObject;
	delete m_renderTarget;
	delete m_pipeline;
	delete m_vertexData;
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

float dT = 0;
void TestSystem::AfterEntityUpdate(float deltaTime)
{
	GraphicsContext *context = XEngine::GetInstance().
		GetInterface<DisplayInterface>(HardwareInterfaceType::Display)->GetGraphicsContext();

	glm::mat4 mvpM = glm::perspective(glm::radians(70.f), 800.f / 600.f, 0.3f, 1000.f)
		* glm::lookAt(glm::vec3(0, 0, 0), glm::vec3(0, 0, -4), glm::vec3(0, 1, 0))
		* glm::translate(glm::mat4(), glm::vec3(0, -2.f, -10.f))
		* glm::rotate(glm::mat4(), dT, glm::vec3(0, 1, 0));

	glm::mat3 nrm = glm::mat3(glm::transpose(glm::inverse(glm::translate(glm::mat4(), glm::vec3(0, -2.f, -10.f))
		* glm::rotate(glm::mat4(), dT, glm::vec3(0, 1, 0)))));

	MeshAssetLoader *mLoader = static_cast<MeshAssetLoader *>(XEngineInstance->GetAssetManager()->GetLoader("Mesh"));

	m_cmdTopBuffer = context->GetGraphicsBufferFromPool();
	m_cmdTopBuffer->BeginRecording();
	m_cmdTopBuffer->BindRenderPass(m_renderTarget, m_renderPass, { glm::vec4(0, 0, 0, 1), glm::vec4(1, 1, 1, 1) }, 0);
	if (m_cubeA.Get().IsFullMeshAvailable())
	{
		m_cubeAVerts = m_cubeA.Get().GetGPUVertices(false);
		m_cubeAInds = m_cubeA.Get().GetGPUIndices(false);

		m_cmdTopBuffer->BindRenderPipeline(m_pipeline);
		m_cmdTopBuffer->BindVertexBuffers(0, { m_cubeAVerts.GetBuffer() }, { 0 });
		m_cmdTopBuffer->BindIndexBuffer(m_cubeAInds.GetBuffer(), false);
		m_cmdTopBuffer->PushShaderConstants(m_shaderData, 0, 0, 1, &mvpM);
		m_cmdTopBuffer->PushShaderConstants(m_shaderData, 1, 0, 1, &nrm);
		m_cmdTopBuffer->DrawIndexed(m_cubeA.Get().GetIndexCount(false), 1, m_cubeAInds.GetPointer()->Pointer / sizeof(int),
			m_cubeAVerts.GetPointer()->Pointer / mLoader->GetMeshMemory(m_cubeA.Get().GetVertexFormatId()).BytesPerVertex, 0);
	}
	m_cmdTopBuffer->EndRenderPass();
	m_cmdTopBuffer->StopRecording();

	XEngine::GetInstance().GetInterface<DisplayInterface>(HardwareInterfaceType::Display)->GetGraphicsContext()
		->SubmitCommands(m_cmdTopBuffer, GraphicsQueueType::Graphics);

	dT += deltaTime;
}

void TestSystem::PostUpdate(float deltaTime, int threadIndex)
{
}
