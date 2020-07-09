#include "pch.h"
#include "TextureAsset.h"

std::string texName = "Texture";

TextureAssetLoader::TextureAssetLoader() : m_tUploadQueue(1e5)
{
	m_context = XEngineInstance->GetInterface<DisplayInterface>(HardwareInterfaceType::Display)->GetGraphicsContext();

	m_tSpec.Add<glm::ivec4>("sizeLevels");
	m_tSpec.Add<VectorDataFormat>("format");
	m_tSpec.Add<ImageType>("imageType");
	m_tSpec.Add<VectorDataFormat>("pixelDataFormat");
	m_tSpec.Add<bool>("keepLowestLevel");
	m_tSpec.AddArray<char>("pixelData");
}

void TextureAssetLoader::CleanupUnusedMemory()
{
}

std::string& TextureAssetLoader::GetAssetType()
{
	return texName;
}

IAsset *TextureAssetLoader::CreateEmpty(UniqueId id)
{
	return new TextureAsset(this, id);
}

void TextureAssetLoader::Preload(IAsset *asset, LoadMemoryPointer header)
{
	TextureAsset *tAsset = static_cast<TextureAsset *>(asset);
	FileSpecImporter importer(m_tSpec, header);

	glm::ivec4& sizeLevels = importer.Get<glm::ivec4>("sizeLevels");
	bool keepLowLevel = importer.Get<bool>("keepLowestLevel");

	tAsset->SetFormat(sizeLevels.x, sizeLevels.y, sizeLevels.z, sizeLevels.w, importer.Get<ImageType>("imageType"),
		importer.Get<VectorDataFormat>("format"));
	tAsset->RetainLowestMipLevelOnUnload(keepLowLevel);
	tAsset->m_pixelFormat = importer.Get<VectorDataFormat>("pixelDataFormat");

	if (keepLowLevel)
	{
		glm::ivec3 mippedSize = glm::max(glm::vec3(sizeLevels) / glm::exp2(static_cast<float>(sizeLevels.w - 1)), glm::vec3(1));
		tAsset->SetData(0, 0, 0, mippedSize.x, mippedSize.y, mippedSize.z, sizeLevels.w - 1, tAsset->m_pixelFormat,
			importer.GetArray<char>("pixelData"));
	}
}

bool TextureAssetLoader::CanLoad(IAsset *asset, LoadMemoryPointer loadData)
{
	return true;
}

std::vector<AssetLoadRange> TextureAssetLoader::Load(IAsset *asset, LoadMemoryPointer loadData)
{
	TextureAsset *tAsset = static_cast<TextureAsset *>(asset);

	AssetLoadRange range;
	range.BulkLoadIndex = reinterpret_cast<int>(loadData);
	range.ByteOffset = 0;
	range.Size = tAsset->GetMipLevelByteCount(range.BulkLoadIndex);

	for (int i = tAsset->GetMaxSizeAndLevelCount().w; i > range.BulkLoadIndex; --i)
		range.ByteOffset += tAsset->GetMipLevelByteCount(i);

	return { range };
}

void TextureAssetLoader::FinishLoad(IAsset *asset, std::vector<AssetLoadRange>& ranges, std::vector<LoadMemoryPointer>& content, LoadMemoryPointer loadData)
{
	TextureAsset *tAsset = static_cast<TextureAsset *>(asset);
	
	for (int i = 0; i < ranges.size(); ++i)
	{
		AssetLoadRange& range = ranges[i];
		LoadMemoryPointer data = content[i];

		PinnedLocalMemory<char> pixelData = XEngineInstance->GetAssetManager()->GetLoadMemory().GetMemory<char>(data);

		glm::ivec3 size = tAsset->GetMipLevelSize(range.BulkLoadIndex);
		tAsset->SetData(0, 0, 0, size.x, size.y, size.z, range.BulkLoadIndex, tAsset->m_pixelFormat, pixelData);
	}
}

void TextureAssetLoader::Copy(IAsset *src, IAsset *dest)
{
	LocalMemoryAllocator& loadMem = XEngineInstance->GetAssetManager()->GetLoadMemory();

	TextureAsset *tSrc = static_cast<TextureAsset *>(src);
	TextureAsset *tDest = static_cast<TextureAsset *>(dest);

	TextureAssetView view = tSrc->GetTexture();

	std::shared_lock<std::shared_mutex> lock(tex->m_availabilityAccess);
	if (tex->m_downloadInProgress.get())
	{
		tex->m_downloadInProgress->Wait(1e16);
		FinishDownload(tex->m_downloadInProgress);
	}

	glm::ivec4 sizeLevels = tSrc->GetMaxSizeAndLevelCount();

	tDest->SetFormat(sizeLevels.x, sizeLevels.y, sizeLevels.z, sizeLevels.w, tSrc->GetType(), tSrc->GetFormat());
	tDest->m_pixelFormat = tSrc->m_pixelFormat;
	tDest->m_keepLowestLevel = tSrc->m_keepLowestLevel;

	GraphicsCommandBuffer *buffer = m_context->GetTransferBufferFromPool();

	buffer->BeginRecording();

	if (view.MinLevelRange != -1)
	{
		tDest->m_view.Image = std::shared_ptr<GraphicsImageObject>(m_context->CreateImage(tDest->m_type, tDest->m_format, glm::ivec3(sizeLevels), 
			view.MaxLevelRange - view.MinLevelRange + 1, ImageUsageBit::Color | ImageUsageBit::TransferDest | ImageUsageBit::Sampling));
		buffer->CopyImageToImage(view.Image.get(), tDest->m_view.Image.get(), glm::ivec3(0), glm::ivec3(0), glm::ivec3(sizeLevels), view.MinLevelRange,
			view.MinLevelRange, view.MaxLevelRange - view.MinLevelRange + 1, true, false, false);
	}

	buffer->StopRecording();

	m_context->SubmitCommands(buffer, GraphicsQueueType::Transfer);

	tDest->m_available = tSrc->m_available;
	tDest->m_buffered.reserve(sizeLevels.w);
	for (LoadMemoryPointer mem : tSrc->m_buffered)
	{
		if (mem)
		{
			PinnedLocalMemory<char> srcData = loadMem.GetMemory<char>(mem);

			LoadMemoryPointer ptr = loadMem.RequestSpace(srcData.GetSize());
			PinnedLocalMemory<char> destData = loadMem.GetMemory<char>(ptr);
			std::memcpy(destData.GetData(), srcData.GetData(), destData.GetSize());

			tDest->m_buffered.push_back(ptr);
		}
		else
		{
			tDest->m_buffered.push_back(nullptr);
		}
	}
}

void TextureAssetLoader::Unload(IAsset *asset)
{
	TextureAsset *tex = static_cast<TextureAsset *>(asset);

	std::shared_lock<std::shared_mutex> lock(tex->m_availabilityAccess);
	if (tex->m_downloadInProgress.get())
	{
		tex->m_downloadInProgress->Wait(1e16);
		FinishDownload(tex->m_downloadInProgress);
	}
}

void TextureAssetLoader::Dispose(IAsset *asset)
{
	TextureAsset *tex = static_cast<TextureAsset *>(asset);

	std::shared_lock<std::shared_mutex> lock(tex->m_availabilityAccess);
	if (tex->m_downloadInProgress.get())
	{
		tex->m_downloadInProgress->Wait(1e16);
		FinishDownload(tex->m_downloadInProgress);
	}
}

void TextureAssetLoader::Export(IAsset *asset, AssetDescriptorPreHeader& preHeader, LoadMemoryPointer& header, LoadMemoryPointer& content)
{
	TextureAsset *tex = static_cast<TextureAsset *>(asset);

	std::shared_lock<std::shared_mutex> lock(tex->m_availabilityAccess);
	if (tex->m_downloadInProgress.get())
	{
		tex->m_downloadInProgress->Wait(1e16);
		FinishDownload(tex->m_downloadInProgress);
	}

	preHeader.AssetSize = 0;

}

TextureAsset::TextureAsset(TextureAssetLoader *loader, UniqueId id)
{
}

UniqueId TextureAsset::GetId()
{
	return UniqueId();
}

std::string& TextureAsset::GetTypeName()
{
	return texName;
}

void TextureAsset::AddRef()
{
}

void TextureAsset::RemoveRef()
{
}

void TextureAsset::SetFormat(int width, int height, int depth, int levels, ImageType type, VectorDataFormat format)
{
}

void TextureAsset::SetData(int x, int y, int z, int width, int height, int depth, int level, VectorDataFormat format, void *data)
{
}

void TextureAsset::SetMaximumStreamingLevel(int lod)
{
}

glm::ivec4 TextureAsset::GetMaxSizeAndLevelCount()
{
	return glm::ivec4();
}

ImageType TextureAsset::GetType()
{
	return ImageType();
}

VectorDataFormat TextureAsset::GetFormat()
{
	return m_format;
}

int TextureAsset::GetBytesPerPixel()
{
	return m_bytesPerPixel;
}

int TextureAsset::GetMipLevelByteCount(int level)
{
	return 0;
}

glm::ivec3 TextureAsset::GetMipLevelSize(int level)
{
	return glm::ivec3();
}

void TextureAsset::RetainLowestMipLevelOnUnload(bool state)
{
	m_keepLowestLevel = state;
}

void TextureAsset::Download()
{
}

bool TextureAsset::AnyDownloadsInProgress()
{
	return false;
}

void TextureAsset::DeleteDuplicatedDataFromGPU()
{
}

void TextureAsset::DeleteDuplicatedDataFromLocal()
{
}

PinnedLocalMemory<char> TextureAsset::GetData(int level)
{
	return PinnedLocalMemory<char>();
}

TextureAssetView TextureAsset::GetTexture()
{
	std::shared_lock<std::shared_mutex> lock(m_availabilityAccess);

	return m_view;
}

void TextureAsset::SetData(int x, int y, int z, int width, int height, int depth, int level, VectorDataFormat format, PinnedLocalMemory<char> data)
{
}

void TextureAsset::BuildView()
{
}
