#pragma once

#include "exports.h"
#include "AssetManager.h"

class OBJMeshImporter : public IFormatImporter
{
public:
	virtual std::vector<std::string>& GetFileExtensions() override;
	virtual void Import(std::string virtualPath, std::string filePath, std::string ext, void *settings) override;
};

