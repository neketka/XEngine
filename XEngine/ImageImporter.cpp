#include "pch.h"
#include "ImageImporter.h"
#include "TextureAsset.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION

#include <stb/stb_image.h>
#include <stb/stb_image_resize.h>

std::vector<std::string>& ImageImporter::GetFileExtensions()
{
	static std::vector<std::string> exts = { "jpeg", "jpg", "png", "gif", "hdr" };
	return exts;
}

void ImageImporter::Import(std::string virtualPath, std::string filePath, std::string ext, void *settings)
{
	TextureAsset *asset = static_cast<TextureAsset *>(XEngineInstance->GetAssetManager()->CreateAssetPtr(virtualPath, "Texture"));

	int32_t w = 0;
	int32_t h = 0;
	int32_t components = 0;

	if (ext == "hdr")
	{
		float *hdrData = stbi_loadf(filePath.c_str(), &w, &h, &components, 4);

		asset->SetFormat(w, h, 1, 1, ImageType::Image2D, VectorDataFormat::R32G32B32A32Float, VectorDataFormat::R32G32B32A32Float);
		asset->SetCPUData(0, hdrData);

		stbi_image_free(hdrData);
	}
	else
	{
		uint8_t *imgData = stbi_load(filePath.c_str(), &w, &h, &components, 4);

		asset->SetFormat(w, h, 1, 1, ImageType::Image2D, VectorDataFormat::R8G8B8A8Unorm, VectorDataFormat::R8G8B8A8Unorm);
		asset->SetCPUData(0, imgData);

		stbi_image_free(imgData);
	}

	asset->LoadAllLevelsFully();
}
