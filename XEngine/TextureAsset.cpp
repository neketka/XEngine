#include "pch.h"
#include "TextureAsset.h"
#include "GLImage.h"

#include <numeric>

std::string texName = "Texture";

TextureAssetLoader::TextureAssetLoader() : m_tUploadQueue(1e5), m_tDownloadQueue(1e5)
{
	m_context = XEngineInstance->GetInterface<DisplayInterface>(HardwareInterfaceType::Display)->GetGraphicsContext();

	m_tSpec.Add<glm::ivec4>("sizeLevels");
	m_tSpec.Add<VectorDataFormat>("format");
	m_tSpec.Add<ImageType>("imageType");
	m_tSpec.Add<VectorDataFormat>("pixelDataFormat");
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

	tAsset->SetFormat(sizeLevels.x, sizeLevels.y, sizeLevels.z, sizeLevels.w, importer.Get<ImageType>("imageType"),
		importer.Get<VectorDataFormat>("format"), importer.Get<VectorDataFormat>("pixelDataFormat"));
}

bool TextureAssetLoader::CanLoad(IAsset *asset, LoadMemoryPointer loadData)
{
	return true;
}

std::vector<AssetLoadRange> TextureAssetLoader::Load(IAsset *asset, LoadMemoryPointer loadData)
{
	TextureAsset *tAsset = static_cast<TextureAsset *>(asset);

	AssetLoadRange range;
	range.BulkLoadIndex = reinterpret_cast<int32_t>(loadData);
	range.ByteOffset = 0;
	range.Size = tAsset->GetMipLevelByteCountOnCPU(range.BulkLoadIndex);

	for (int32_t i = tAsset->GetMaxSizeAndLevelCount().w - 1; i > range.BulkLoadIndex; --i)
		range.ByteOffset += tAsset->GetMipLevelByteCountOnCPU(i);

	return { range };
}

void TextureAssetLoader::FinishLoad(IAsset *asset, std::vector<AssetLoadRange>& ranges, std::vector<LoadMemoryPointer>& content, LoadMemoryPointer loadData)
{
	if (ranges.empty())
		return;

	TextureAsset *tAsset = static_cast<TextureAsset *>(asset);
	
	for (int32_t i = 0; i < ranges.size(); ++i)
	{
		AssetLoadRange& range = ranges[i];
		LoadMemoryPointer data = content[i];

		PinnedLocalMemory<char> pixelData = XEngineInstance->GetAssetManager()->GetLoadMemory().GetMemory<char>(data);

		if (tAsset->m_levelsStreamingToCpuStatus[range.BulkLoadIndex])
			tAsset->SetCPUData(range.BulkLoadIndex, pixelData.GetData());
	}
}

void TextureAssetLoader::Copy(IAsset *src, IAsset *dest)
{
	LocalMemoryAllocator& assetMem = XEngineInstance->GetAssetManager()->GetAssetMemory();

	TextureAsset *tSrc = static_cast<TextureAsset *>(src);
	TextureAsset *tDest = static_cast<TextureAsset *>(dest);

	std::lock_guard lock(tDest->m_streamingMutex);

	glm::ivec4 sizeLevels = tSrc->GetMaxSizeAndLevelCount();

	tDest->SetFormat(sizeLevels.x, sizeLevels.y, sizeLevels.z, sizeLevels.w, tSrc->GetType(), tSrc->GetFormat(), tSrc->m_pixelFormat);

	tDest->m_levelsOnCpu = tSrc->m_levelsOnGpu;
	tDest->m_levelsOnGpu = tSrc->m_levelsOnGpu;
	tDest->m_gpuImageBounds = tSrc->m_gpuImageBounds;
	tDest->m_validGpuImageBounds = tSrc->m_validGpuImageBounds;

	tDest->m_image = m_context->CreateImage(tDest->m_type, tDest->m_format, tDest->GetMipLevelSize(tDest->m_gpuImageBounds.x),
		tDest->m_gpuImageBounds.y - tDest->m_gpuImageBounds.x + 1, ImageUsageBit::TransferDest | ImageUsageBit::Color);

	GraphicsCommandBuffer *buffer = m_context->GetTransferBufferFromPool();
	GraphicsSyncObject *obj = m_context->CreateSync(false);

	buffer->BeginRecording();

	for (int32_t level = 0; level < sizeLevels.w; ++level)
	{
		if (tDest->m_levelsOnCpu[level])
		{
			PinnedLocalMemory<char> src = assetMem.GetMemory<char>(tSrc->m_levelsOnCpuData[level]);
			tDest->SetCPUData(level, src.GetData());
		}
		if (tDest->m_levelsOnGpu[level])
		{
			buffer->CopyImageToImage(tSrc->m_image, tDest->m_image, glm::ivec3(0), glm::ivec3(0), tDest->GetMipLevelSize(level),
				level, level, 1, true, false, false);
		}
	}

	buffer->SignalFence(obj);

	buffer->StopRecording();

	m_context->SubmitCommands(buffer, GraphicsQueueType::Transfer);

	obj->Wait(1e16);
	delete obj;
}

void TextureAssetLoader::Unload(IAsset *asset)
{
	TextureAsset *tex = static_cast<TextureAsset *>(asset);

	LocalMemoryAllocator& assetMem = XEngineInstance->GetAssetManager()->GetAssetMemory();
	
	std::lock_guard sLock(tex->m_streamingMutex);
	while (std::any_of(tex->m_levelsStreamingToCpuStatus.begin(), tex->m_levelsStreamingToCpuStatus.end(), [](bool arg) { return arg; }));
	for (std::shared_ptr<GraphicsSyncObject>& obj : tex->m_levelsStreamingToGpu)
	{
		obj->Wait(1e16);
		obj = nullptr;
	}
	if (tex->m_image)
	{
		delete tex->m_image;
		tex->m_image = nullptr;
	}
	for (AssetMemoryPointer& ptr : tex->m_levelsOnCpuData)
	{
		if (ptr)
		{
			assetMem.FreeSpace(ptr);
			ptr = nullptr;
		}
	}
	tex->m_gpuImageBounds = glm::ivec2(-1, -1);
	tex->m_validGpuImageBounds = glm::ivec2(-1, -1);

	int lvls = tex->m_levelsOnCpu.size();
	
	tex->m_levelsOnCpu.assign(lvls, false);
	tex->m_levelsOnGpu.assign(lvls, false);
	tex->m_levelsStreamingToCpuStatus.assign(lvls, false);
	tex->m_levelsStreamingToGpuStatus.assign(lvls, false);
}

void TextureAssetLoader::Dispose(IAsset *asset)
{
	TextureAsset *tex = static_cast<TextureAsset *>(asset);

	Unload(asset);

	delete tex;
}

void TextureAssetLoader::Export(IAsset *asset, AssetDescriptorPreHeader& preHeader, LoadMemoryPointer& header, LoadMemoryPointer& content,
	std::vector<AssetLoadRange>& ranges)
{
	LocalMemoryAllocator& loadMem = XEngineInstance->GetAssetManager()->GetLoadMemory();
	LocalMemoryAllocator& assetMem = XEngineInstance->GetAssetManager()->GetAssetMemory();

	TextureAsset *tex = static_cast<TextureAsset *>(asset);
	FileSpecExporter exp(m_tSpec);

	header = exp.AllocateSpace();
	int32_t size = std::accumulate(tex->m_mipPixelBytesSizes.begin(), tex->m_mipPixelBytesSizes.end(), 0);
	content = loadMem.RequestSpace(size);

	preHeader.HeaderSize = exp.GetTotalByteSize();
	preHeader.AssetSize = size;
	strcpy_s(preHeader.AssetType, texName.c_str());

	exp.Set<glm::ivec4>("sizeLevels", tex->m_sizeLevels);
	exp.Set<ImageType>("imageType", tex->m_type);
	exp.Set<VectorDataFormat>("format", tex->m_format);
	exp.Set<VectorDataFormat>("pixelDataFormat", tex->m_pixelFormat);

	PinnedLocalMemory<char> dest = loadMem.GetMemory<char>(content);

	uint32_t ptr = 0;
	for (int32_t i = tex->m_sizeLevels.w - 1; i <= 0; ++i)
	{
		PinnedLocalMemory<char> src = assetMem.GetMemory<char>(tex->m_levelsOnCpuData[i]);

		std::memcpy(dest.GetData() + ptr, src.GetData(), src.GetSize());

		AssetLoadRange range;
		range.ByteOffset = ptr;
		range.Size = src.GetSize();

		ranges.push_back(range);

		ptr += src.GetSize();
	}
}

std::shared_ptr<GraphicsSyncObject> TextureAssetLoader::UploadTextureData(GraphicsCommandBuffer *buf, GraphicsImageObject *obj, int32_t x, int32_t y, int32_t z,
	int32_t width, int32_t height, int32_t depth, int32_t level, VectorDataFormat format, ImageType type, void *data)
{
	GraphicsBufferImageCopyRegion region;

	region.BufferOffset = 0;
	region.BufferRowLength = 0;
	region.BufferImageHeight = 0;
	region.Level = level;
	region.Offset = glm::ivec3(x, y, z);
	region.Size = glm::ivec3(width, height, depth);

	std::shared_ptr<GraphicsSyncObject> sync = m_tUploadQueue.Upload<char>(buf, reinterpret_cast<char *>(data), type, obj, region,
		CalcMemUsage(width, height, depth, level, format), format);

	return sync;
}

uint32_t TextureAssetLoader::CalcMemUsage(int32_t width, int32_t height, int32_t depth, int32_t level, VectorDataFormat format)
{
	VectorFormatDesc desc = GetFormatDesc(format);
	return (desc.RBits + desc.GBits + desc.BBits + desc.ABits) * std::max<int32_t>(width / std::exp2(level), 1) *
		std::max<int32_t>(height / std::exp2(level), 1) * std::max<int32_t>(depth / std::exp2(level), 1) / 8;
}

TextureAsset::TextureAsset(TextureAssetLoader *loader, UniqueId id) : m_loader(loader), m_id(id), m_refCount(0),
	m_gpuImageBounds(-1, -1), m_validGpuImageBounds(-1, -1), m_image(nullptr)
{
	m_context = XEngineInstance->GetInterface<DisplayInterface>(HardwareInterfaceType::Display)->GetGraphicsContext();
	m_copyOldImg = m_context->CreateSync(false);
}

UniqueId TextureAsset::GetId()
{
	return m_id;
}

std::string& TextureAsset::GetTypeName()
{
	return texName;
}

void TextureAsset::AddRef()
{
	++m_refCount;
}

void TextureAsset::RemoveRef()
{
	--m_refCount;
	if (m_refCount == 0)
	{
		XEngineInstance->GetAssetManager()->PushUnloadRequest(m_loader, this);
	}
}

void TextureAsset::SetFormat(int32_t width, int32_t height, int32_t depth, int32_t levels, ImageType type, VectorDataFormat format,
	VectorDataFormat pixelFormat)
{
	m_loader->Unload(this);

	m_sizeLevels = glm::ivec4(width, height, depth, levels);
	m_type = type;
	m_format = format;
	m_pixelFormat = pixelFormat;

	VectorFormatDesc desc = GetFormatDesc(format);
	m_bytesPerPixel = (desc.RBits + desc.GBits + desc.BBits + desc.ABits) / 8;

	VectorFormatDesc pixelDesc = GetFormatDesc(pixelFormat);
	m_bytesPerPixelCPU = (desc.RBits + desc.GBits + desc.BBits + desc.ABits) / 8;

	m_mipBytesSizes.resize(levels);
	m_mipPixelBytesSizes.resize(levels);

	m_levelsOnCpu.resize(levels);
	m_levelsOnGpu.resize(levels);
	m_levelsStreamingToCpuStatus.resize(levels);
	m_levelsStreamingToGpuStatus.resize(levels);
	m_levelsStreamingToGpu.resize(levels);
	m_levelsOnCpuData.resize(levels);

	for (int32_t i = 0; i < levels; ++i)
	{
		glm::ivec3 size = GetMipLevelSize(i);
		int innerProduct = size.x * size.y * size.z;
		m_mipBytesSizes[i] = innerProduct * m_bytesPerPixel;
		m_mipPixelBytesSizes[i] = innerProduct * m_bytesPerPixelCPU;
	}
}

void TextureAsset::SetCPUData(int32_t level, void *data)
{
	std::lock_guard lock(m_streamingMutex);

	LocalMemoryAllocator& alloc = XEngineInstance->GetAssetManager()->GetAssetMemory();
	m_levelsOnCpuData[level] = alloc.RequestSpace(m_mipPixelBytesSizes[level]);

	glm::ivec3 size = GetMipLevelSize(level);

	PinnedLocalMemory<char> mem = alloc.GetMemory<char>(m_levelsOnCpuData[level]);

	std::memcpy(mem.GetData(), data, mem.GetSize());

	m_levelsOnCpu[level] = true;
	m_levelsStreamingToCpuStatus[level] = false;
}

glm::ivec4 TextureAsset::GetMaxSizeAndLevelCount()
{
	return m_sizeLevels;
}

ImageType TextureAsset::GetType()
{
	return m_type;
}

VectorDataFormat TextureAsset::GetFormat()
{
	return m_format;
}

int32_t TextureAsset::GetBytesPerPixelGPU()
{
	return m_bytesPerPixel;
}

int32_t TextureAsset::GetBytesPerPixelCPU()
{
	return m_bytesPerPixelCPU;
}

int32_t TextureAsset::GetMipLevelByteCountOnGPU(int32_t level)
{
	return m_mipBytesSizes[level];
}

int32_t TextureAsset::GetMipLevelByteCountOnCPU(int32_t level)
{
	return m_mipPixelBytesSizes[level];
}

glm::ivec3 TextureAsset::GetMipLevelSize(int32_t level)
{
	if (m_type == ImageType::Image1DArray)
		return glm::max<int32_t>(glm::ivec3(glm::vec3(m_sizeLevels) / glm::exp2<float>(level)), glm::ivec3(1, m_sizeLevels.y, 1));
	else if (m_type == ImageType::Image2DArray || m_type == ImageType::ImageCubemapArray || m_type == ImageType::ImageCubemap)
		return glm::max<int32_t>(glm::ivec3(glm::vec3(m_sizeLevels) / glm::exp2<float>(level)), glm::ivec3(1, 1, m_sizeLevels.z));
	else
		return glm::max<int32_t>(glm::ivec3(glm::vec3(m_sizeLevels) / glm::exp2<float>(level)), glm::ivec3(1));
}

void TextureAsset::LoadAllLevelsFully()
{
	for (int32_t level = 0; level < m_sizeLevels.w; ++level)
	{
		if (!m_levelsOnCpu[level])
			StreamLevelToCPU(level);
	}
	while (std::any_of(GetLevelsStreamingToCPU().begin(), GetLevelsStreamingToCPU().end(), [](bool arg) { return arg; }));

	GraphicsCommandBuffer *buf = m_context->GetTransferBufferFromPool();
	GraphicsSyncObject *sync = m_context->CreateSync(false);

	buf->BeginRecording();

	BuildGPUImage(buf, 0, m_sizeLevels.w - 1);
	SetValidGPUImageBounds(0, m_sizeLevels.w - 1);

	for (int32_t level = 0; level < m_sizeLevels.w; ++level)
	{
		if (!m_levelsOnGpu[level])
		{
			StreamLevelToGPU(buf, level);
			m_levelsOnGpu[level] = true;
		}
	}

	buf->SignalFence(sync);

	buf->StopRecording();

	m_context->SubmitCommands(buf, GraphicsQueueType::Transfer);

	sync->Wait(1e16);

	delete sync;
}

bool TextureAsset::StreamLevelToCPU(int32_t level)
{
	if (!m_levelsStreamingToCpuStatus[level])
	{
		std::lock_guard streamingLock(m_streamingMutex);

		if (m_levelsOnCpu[level])
		{
			XEngineInstance->GetAssetManager()->GetAssetMemory().FreeSpace(m_levelsOnCpuData[level]);
			m_levelsOnCpu[level] = false;
		}

		XEngineInstance->GetAssetManager()->PushLoadRequest(m_loader, this, reinterpret_cast<LoadMemoryPointer>(level));
		m_levelsStreamingToCpuStatus[level] = true;
		return true;
	}
	else
	{
		XEngineInstance->LogMessage("Cannot stream mipmap level that is already being streamed!", LogMessageType::Error);
		return false;
	}
}

bool TextureAsset::DeleteCPULevel(int32_t level)
{
	std::lock_guard streamingLock(m_streamingMutex);
	if (m_levelsStreamingToCpuStatus[level])
	{
		XEngineInstance->LogMessage("Cannot delete mipmap level that is being streamed!", LogMessageType::Error);
		return false;
	}
	if (m_levelsOnCpu[level])
	{
		XEngineInstance->GetAssetManager()->GetAssetMemory().FreeSpace(m_levelsOnCpuData[level]);
		m_levelsOnCpuData[level] = nullptr;
	}
	return true;
}

PinnedLocalMemory<char> TextureAsset::GetCPULevelMemory(int32_t level)
{
	return XEngineInstance->GetAssetManager()->GetAssetMemory().GetMemory<char>(m_levelsOnCpuData[level]);
}

std::vector<bool>& TextureAsset::GetLevelsOnCPU()
{
	std::lock_guard streamingLock(m_streamingMutex);
	return m_levelsOnCpu;
}

std::vector<bool>& TextureAsset::GetLevelsStreamingToCPU()
{
	std::lock_guard streamingLock(m_streamingMutex);
	return m_levelsStreamingToCpuStatus;
}

void TextureAsset::BuildGPUImage(GraphicsCommandBuffer *cmdBuf, int32_t minLevel, int32_t maxLevel)
{
	if (m_oldImg)
	{
		m_copyOldImg->Wait(1e6);
		delete m_oldImg;
		m_oldImg = nullptr;
	}

	if (minLevel < 0)
	{
		if (m_image)
			delete m_image;
		m_image = nullptr;
		m_view = nullptr;
	}

	GraphicsImageObject *newImg = m_context->CreateImage(m_type, m_format, GetMipLevelSize(minLevel), maxLevel - minLevel + 1, 
		ImageUsageBit::TransferDest | ImageUsageBit::Color);

	if (m_image)
	{
		if (m_validGpuImageBounds.x != -1)
		{
			int32_t minSrcLevel = glm::max(minLevel, m_validGpuImageBounds.x) - m_gpuImageBounds.x;
			int32_t maxSrcLevel = glm::min(maxLevel, m_validGpuImageBounds.y) - m_gpuImageBounds.x;

			int32_t destDiff = m_gpuImageBounds.x - minLevel;

			for (int32_t i = minSrcLevel; i <= maxSrcLevel; ++i)
			{
				cmdBuf->ResetFence(m_copyOldImg);
				cmdBuf->CopyImageToImage(m_image, newImg, glm::ivec3(0), glm::ivec3(0), GetMipLevelSize(i), i, i + destDiff, 1, 
					true, false, false);
				cmdBuf->SignalFence(m_copyOldImg);
			}
		}
		m_oldImg = m_image;
	}
	else
	{
		m_gpuImageBounds = glm::ivec2(minLevel, maxLevel);
		m_validGpuImageBounds = glm::ivec2(-1, -1);
	}
	m_image = newImg;
}

bool TextureAsset::StreamLevelToGPU(GraphicsCommandBuffer *cmdBuf, int32_t level)
{
	if (m_levelsOnCpu[level])
	{
		glm::ivec3 size = GetMipLevelSize(level);
		PinnedLocalMemory<char> data = XEngineInstance->GetAssetManager()->GetAssetMemory().GetMemory<char>(m_levelsOnCpuData[level]);
		m_loader->UploadTextureData(cmdBuf, m_image, 0, 0, 0, size.x, size.y, size.z, level, m_pixelFormat, m_type, data.GetData());
		m_shouldCheckGPUStream = true;
	}
	else
	{
		XEngineInstance->LogMessage("Cannot stream mipmap level not present on CPU!", LogMessageType::Error);
		return false;
	}
	return true;
}

bool TextureAsset::InvalidateGPULevel(int32_t level)
{
	CheckGpuStream();

	if (!m_levelsStreamingToGpuStatus[level])
	{
		m_levelsOnGpu[level] = false;
	}
	else
	{
		XEngineInstance->LogMessage("Cannot delete mipmap level streaming to GPU!", LogMessageType::Error);
		return false;
	}

	return true;
}

std::vector<bool>& TextureAsset::GetLevelsOnGPU()
{
	CheckGpuStream();
	return m_levelsOnGpu;
}

std::vector<bool>& TextureAsset::GetLevelsStreamingToGPU()
{
	CheckGpuStream();
	return m_levelsStreamingToGpuStatus;
}

void TextureAsset::SetValidGPUImageBounds(int32_t minLevel, int32_t maxLevel)
{
	m_validGpuImageBounds.x = glm::max(minLevel, m_gpuImageBounds.x);
	m_validGpuImageBounds.y = glm::min(maxLevel, m_gpuImageBounds.y);

	int32_t min = m_validGpuImageBounds.x - m_gpuImageBounds.x;
	int32_t max = m_validGpuImageBounds.y - m_gpuImageBounds.x;
	int32_t layers = m_type == ImageType::Image3D ? 1 : m_sizeLevels.z;

	m_view = std::shared_ptr<GraphicsImageView>(m_image->CreateView(m_type, m_format, min, max - min + 1, 0, layers,
		ImageSwizzleComponent::Red, ImageSwizzleComponent::Green, ImageSwizzleComponent::Blue, ImageSwizzleComponent::Alpha));
}

std::shared_ptr<GraphicsImageView> TextureAsset::GetImage()
{
	return m_view;
}

glm::ivec2 TextureAsset::GetImageLevelRange()
{
	return m_gpuImageBounds;
}

glm::ivec2 TextureAsset::GetImageValidLevelRange()
{
	return m_validGpuImageBounds;
}

void TextureAsset::CheckGpuStream()
{
	if (m_shouldCheckGPUStream)
	{
		m_shouldCheckGPUStream = false;
		int32_t level = 0;
		for (std::shared_ptr<GraphicsSyncObject>& ptr : m_levelsStreamingToGpu)
		{
			if (ptr)
			{
				if (ptr->GetCurrentStatus())
				{
					ptr = nullptr;
					m_levelsStreamingToGpuStatus[level] = false;
					m_levelsOnGpu[level] = true;
				}
				else m_shouldCheckGPUStream = true;
			}
			++level;
		}
	}
}
