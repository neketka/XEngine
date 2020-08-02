#include "pch.h"
#include "GLSLImporter.h"
#include <shaderc/shaderc.hpp>
#include <fstream>
#include <streambuf>
#include <filesystem>

#include "ShaderAsset.h"

class GLSLIncluder : public shaderc::CompileOptions::IncluderInterface
{
public:
	GLSLIncluder(ShaderAssetLoader *loader) : m_loader(loader)
	{
	}
	virtual shaderc_include_result *GetInclude(const char *requested_source, shaderc_include_type type, const char *requesting_source, size_t include_depth) override
	{
		shaderc_include_result *r = new shaderc_include_result;
		if (type == shaderc_include_type::shaderc_include_type_relative)
		{
			std::ifstream t(requested_source);
			std::string str((std::istreambuf_iterator<char>(t)),
				std::istreambuf_iterator<char>());

			auto absPath = std::filesystem::absolute(std::filesystem::path(requested_source));
			
			r->content = new char[str.size() + 1];
			r->content_length = str.size();
			r->source_name = new char[absPath.string().size() + 1];
			r->source_name_length = absPath.string().size();

			std::strcpy(const_cast<char *>(r->content), str.c_str());
			std::strcpy(const_cast<char *>(r->source_name), absPath.string().c_str());
		}
		else
		{
			auto absPath = std::filesystem::absolute(std::filesystem::path(requested_source));
			std::string str = m_loader->ResolveIncludedPath(requested_source);

			r->content = new char[str.size() + 1];
			r->content_length = str.size();
			r->source_name = new char[absPath.string().size() + 1];
			r->source_name_length = absPath.string().size();

			std::strcpy(const_cast<char *>(r->content), str.c_str());
			std::strcpy(const_cast<char *>(r->source_name), absPath.string().c_str());
		}

		return r;
	}
	virtual void ReleaseInclude(shaderc_include_result *data) override
	{
		delete[] data->content;
		delete[] data->source_name;
	}
private:
	ShaderAssetLoader *m_loader;
};

std::vector<std::string> glslImporterExts = {
	"vsh", "fsh", "csh", "gsh", "tcsh", "tesh"
};

std::vector<std::string>& GLSLImporter::GetFileExtensions()
{
	return glslImporterExts;
}

void GLSLImporter::Import(std::string virtualPath, std::string filePath, std::string ext, void *settings)
{
	shaderc::Compiler compiler;
	shaderc::CompileOptions options;

	options.SetTargetEnvironment(shaderc_target_env::shaderc_target_env_opengl, 0);
	
	ShaderStageBit stage = ShaderStageBit::Vertex;
	shaderc_shader_kind kind = shaderc_shader_kind::shaderc_glsl_infer_from_source;
	if (ext == "vsh")
	{
		kind = shaderc_shader_kind::shaderc_glsl_vertex_shader;
		stage = ShaderStageBit::Vertex;
	}
	else if (ext == "fsh")
	{
		kind = shaderc_shader_kind::shaderc_glsl_fragment_shader;
		stage = ShaderStageBit::Fragment;
	}
	else if (ext == "csh")
	{
		kind = shaderc_shader_kind::shaderc_glsl_compute_shader;
		stage = ShaderStageBit::Compute;
	}
	else if (ext == "gsh")
	{
		kind = shaderc_shader_kind::shaderc_glsl_geometry_shader;
		stage = ShaderStageBit::Geometry;
	}
	else if (ext == "tcsh")
	{
		kind = shaderc_shader_kind::shaderc_glsl_tess_control_shader;
		stage = ShaderStageBit::TessControl;
	}
	else if (ext == "tesh")
	{
		kind = shaderc_shader_kind::shaderc_glsl_tess_evaluation_shader;
		stage = ShaderStageBit::TessEval;
	}

	std::ifstream t(filePath);
	std::string str((std::istreambuf_iterator<char>(t)),
		std::istreambuf_iterator<char>());

	auto preprocess = compiler.PreprocessGlsl(str, kind, filePath.c_str(), options);
	auto result = compiler.CompileGlslToSpv(std::string(preprocess.cbegin(), preprocess.cend()), kind, filePath.c_str(), options);

	ShaderAsset *asset = static_cast<ShaderAsset *>(XEngineInstance->GetAssetManager()->CreateAssetPtr(virtualPath, "Shader"));

	asset->SetStage(stage);
	asset->SetShaderCodeMode(ShaderAssetCode::GlSpirv);
	asset->UploadCode(const_cast<uint32_t *>(result.begin()), (result.end() - result.begin()) * sizeof(uint32_t));
}
