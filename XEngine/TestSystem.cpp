#include "pch.h"
#include "TestSystem.h"
#include "GLShaderCompilation.h"
#include "testimage.h"
#include <glm/gtc/matrix_transform.hpp>

#include <shaderc/shaderc.hpp>

const char *vShader = R"glsl(

#version 460

out gl_PerVertex
{
  vec4 gl_Position;
  float gl_PointSize;
  float gl_ClipDistance[];
};
layout(location = 0) out vec3 normal;
layout(location = 1) out vec2 uv;
layout(location = 0) in vec4 posr;
layout(location = 1) in vec4 normalg;
layout(location = 2) in vec4 tangentb;
layout(location = 3) in vec4 uv0a;
layout(location = 0) uniform mat4 mvp;
layout(location = 1) uniform mat3 nrm;
void main()
{
	normal = nrm * normalg.xyz;
	uv = uv0a.xy;
	gl_Position = mvp * vec4(posr.xyz, 1);
}
)glsl";

const char *fShader = R"glsl(

#version 460

layout(location = 0) out vec4 color;
layout(location = 0) in vec3 normal;
layout(location = 1) in vec2 uv;
layout(location = 0) uniform sampler2D tex;
void main()
{
	float nDotl = dot(normal, -normalize(vec3(-0.5, -0.5, -1)));
	color = vec4(texture(tex, uv).xyz * nDotl, 1);
}
)glsl";

class TestEntityData
{
public:
	TestComponent *Component;
};

void TestSystem::Initialize()
{
	GraphicsContext *context = XEngine::GetInstance().
		GetInterface<DisplayInterface>(HardwareInterfaceType::Display)->GetGraphicsContext();

	XEngineInstance->GetAssetManager()->PreloadAssetBundle("./Assets/testmodels.xasset");
	XEngineInstance->GetAssetManager()->PreloadAssetBundle("./Assets/testimage.xasset");

	UniqueId cubeAId = XEngineInstance->GetAssetManager()->GetIdByPath("assets/cube");
	m_cubeA = XEngineInstance->GetAssetManager()->GetAsset<MeshAsset>(cubeAId);
	m_cubeA->LoadFullMesh();

	UniqueId texId = XEngineInstance->GetAssetManager()->GetIdByPath("assets/testimage");
	m_texture = XEngineInstance->GetAssetManager()->GetAsset<TextureAsset>(texId);
	m_texture->LoadAllLevelsFully();

	shaderc::Compiler compiler;
	shaderc::CompileOptions options;

	options.SetTargetEnvironment(shaderc_target_env::shaderc_target_env_opengl, 0);

	auto vshaderResult = compiler.CompileGlslToSpv(vShader, shaderc_shader_kind::shaderc_vertex_shader, "vshader", options);
	auto fshaderResult = compiler.CompileGlslToSpv(fShader, shaderc_shader_kind::shaderc_fragment_shader, "fshader", options);

	std::vector<uint32_t> vshaderCode(vshaderResult.begin(), vshaderResult.end());
	std::vector<uint32_t> fshaderCode(fshaderResult.begin(), fshaderResult.end());

	m_shaderVertexCode = new GLShaderCode(vshaderCode, ShaderStageBit::Vertex);
	m_shaderFragmentCode = new GLShaderCode(fshaderCode, ShaderStageBit::Fragment);

	m_vshader = context->CreateShader(m_shaderVertexCode);
	m_fshader = context->CreateShader(m_shaderFragmentCode);

	m_specData = new GLShaderSpecialization("main", {});

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

	GraphicsShaderResourceViewData srvData;
	srvData.Binding = 2;
	srvData.Count = 1;
	srvData.Stages = ShaderStageBit::Fragment;
	srvData.Type = ShaderResourceType::ImageAndSampler;

	m_texInst = context->CreateShaderResourceInstance(srvData);

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
	nrm.Unsigned = false;
	nrm.Offset = 1;
	nrm.VectorLength = 9;
	nrm.Stages = ShaderStageBit::Vertex;
	nrm.ElementCount = 9;

	m_shaderData = context->CreateShaderDataSet({ srvData }, { mvp, nrm });
	ps.ShaderDataSet = m_shaderData;

	context->UpdateShaderImageSamplerResourceInstance(m_texInst, { m_texture->GetImage().get() }, { m_sampler });

	MeshAssetLoader *mLoader = static_cast<MeshAssetLoader *>(XEngineInstance->GetAssetManager()->GetLoader("Mesh"));

	mLoader->SetVertexInputState(ps.VertexInputState, m_cubeA->GetVertexFormatId());
	m_pipeline = context->CreateGraphicsPipeline(ps);
}

void TestSystem::Destroy()
{
	GraphicsContext *context = XEngine::GetInstance().GetInterface<DisplayInterface>(HardwareInterfaceType::Display)
		->GetGraphicsContext();
	context->SyncWithCommandSubmissionThread();

	delete m_sampler;
	delete m_texInst;
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

int32_t instance = 0;
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
		for (int32_t i = 0; i < 100; ++i)
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
		* glm::translate(glm::mat4(), glm::vec3(0, std::sinf(dT) * 2, -10.f))
		* glm::rotate(glm::mat4(), dT, glm::vec3(0, 1, 0));

	glm::mat3 nrm = glm::mat3(glm::transpose(glm::inverse(glm::rotate(glm::mat4(), dT, glm::vec3(0, 1, 0)))));

	MeshAssetLoader *mLoader = static_cast<MeshAssetLoader *>(XEngineInstance->GetAssetManager()->GetLoader("Mesh"));

	m_cmdTopBuffer = context->GetGraphicsBufferFromPool();
	m_cmdTopBuffer->BeginRecording();
	m_cmdTopBuffer->BindRenderPass(m_renderTarget, m_renderPass, { glm::vec4(0, 0, 0, 1), glm::vec4(1, 1, 1, 1) }, 0);
	if (m_cubeA->IsFullMeshAvailable())
	{
		m_cubeAVerts = m_cubeA->GetGPUVertices();
		m_cubeAInds = m_cubeA->GetGPUIndices();

		m_cmdTopBuffer->BindRenderPipeline(m_pipeline);
		m_cmdTopBuffer->BindVertexBuffers(0, { m_cubeAVerts.GetBuffer() }, { 0 });
		m_cmdTopBuffer->BindIndexBuffer(m_cubeAInds.GetBuffer(), false);
		m_cmdTopBuffer->BindRenderShaderResourceInstance(m_shaderData, m_texInst, 0, 0);
		m_cmdTopBuffer->PushShaderConstants(m_shaderData, 0, 0, 1, &mvpM);
		m_cmdTopBuffer->PushShaderConstants(m_shaderData, 1, 0, 1, &nrm);
		m_cmdTopBuffer->DrawIndexed(m_cubeA->GetIndexCount(), 1, m_cubeAInds.GetPointer()->Pointer / sizeof(int32_t),
			m_cubeAVerts.GetPointer()->Pointer / mLoader->GetMeshMemory(m_cubeA->GetVertexFormatId()).BytesPerVertex, 0);
	}
	m_cmdTopBuffer->EndRenderPass();
	m_cmdTopBuffer->StopRecording();

	XEngine::GetInstance().GetInterface<DisplayInterface>(HardwareInterfaceType::Display)->GetGraphicsContext()
		->SubmitCommands(m_cmdTopBuffer, GraphicsQueueType::Graphics);

	dT += deltaTime;
}

void TestSystem::PostUpdate(float deltaTime, int32_t threadIndex)
{
}
