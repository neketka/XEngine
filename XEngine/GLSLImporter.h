#pragma once

#include "AssetManager.h"

class GLSLImporter : public IFormatImporter
{
public:
	virtual std::vector<std::string>& GetFileExtensions() override;
	virtual void Import(std::string virtualPath, std::string filePath, std::string ext, void *settings) override;
};

