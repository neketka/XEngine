#pragma once

#include "AssetManager.h"

class ImageImportSettings
{
public:
	int MipmapLevels = 1;
	int ArrayLevels = 1;
	bool Cubemap = false;
};

class ImageImporter : public IFormatImporter
{
public:
	ImageImporter() {}
	virtual std::vector<std::string>& GetFileExtensions() override;
	virtual void Import(std::string virtualPath, std::string filePath, std::string ext, void *settings) override;
};

