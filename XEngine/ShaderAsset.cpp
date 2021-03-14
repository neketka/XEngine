#include "pch.h"
#include "ShaderAsset.h"
#include "GLShaderData.h"

#include <set>

std::string shaderAssetTypeName = "Shader";

ShaderAsset::ShaderAsset(ShaderAssetLoader *loader, UniqueId id) : m_loader(loader), m_id(id),
    m_shaderCode(nullptr), m_glCodeLength(0), m_refCounter(0), m_vkCode(nullptr), m_vkCodeLength(0)
{
    LocalMemoryAllocator& alloc = XEngineInstance->GetAssetManager()->GetAssetMemory();

    m_glCode = alloc.RequestSpace(0);
    m_vkCode = alloc.RequestSpace(0);
}

UniqueId ShaderAsset::GetId()
{
    return m_id;
}

std::string& ShaderAsset::GetTypeName()
{
    return shaderAssetTypeName;
}

void ShaderAsset::AddRef()
{
    ++m_refCounter;
}

void ShaderAsset::RemoveRef()
{
    --m_refCounter;
    if (m_refCounter == 0)
    {
        XEngineInstance->GetAssetManager()->PushUnloadRequest(m_loader, this);
    }
}

void ShaderAsset::SetShaderCodeMode(ShaderAssetCode code)
{
    m_mode = code;
}

void ShaderAsset::UploadCode(void *code, int32_t dataSize)
{
    LocalMemoryAllocator& alloc = XEngineInstance->GetAssetManager()->GetAssetMemory();

    if (m_shaderCode)
    {
        delete m_shaderCode;
        delete m_shader;
    }

    if (m_mode == ShaderAssetCode::GlSpirv)
    {
        alloc.FreeSpace(m_glCode);
        if (m_shaderCode)
            delete m_shaderCode;
        m_glCode = alloc.RequestSpace(dataSize);
        PinnedLocalMemory<char> str = alloc.GetMemory<char>(m_glCode);
        std::memcpy(str.GetData(), code, dataSize);

        m_glCodeLength = dataSize;

        m_shaderCode = new GLShaderCode(std::vector<uint32_t>(str.GetData(), str.GetData() + dataSize / 4), m_stage);
        m_shader = XEngineInstance->GetInterface<DisplayInterface>(HardwareInterfaceType::Display)->GetGraphicsContext()
            ->CreateShader(m_shaderCode);
    }
}

void ShaderAsset::AddSpecializationInformation(std::string flagName, std::vector<std::pair<uint32_t, uint32_t>> specialization)
{
    m_flags[flagName] = specialization;
}

GraphicsSpecificSpecializationData *ShaderAsset::CreateSpecialization(std::vector<std::string> flags, std::string entryPoint)
{
    std::set<std::pair<uint32_t, uint32_t>> defineSet;

    for (std::string& str : flags)
    {
        std::vector<std::pair<uint32_t, uint32_t>>& defs = m_flags[str];
        defineSet.insert(defs.begin(), defs.end());
    }
    if (m_mode == ShaderAssetCode::GlSpirv)
        return new GLShaderSpecialization(entryPoint, std::vector<std::pair<uint32_t, uint32_t>>(defineSet.begin(), defineSet.end()));
    return nullptr;
}

GraphicsShader *ShaderAsset::GetShader()
{
    return m_shader;
}

void ShaderAsset::SetStage(ShaderStageBit stage)
{
    m_stage = stage;
}

ShaderStageBit ShaderAsset::GetStage()
{
    return m_stage;
}

ShaderAssetLoader::ShaderAssetLoader()
{
    m_spec.Add<ShaderStageBit>("stage");
    m_spec.AddStringArray("flagKeys");
    m_spec.AddArray<uint32_t>("flagArraySizes");
    m_spec.AddArray<std::pair<uint32_t, uint32_t>>("flagFlattenedValues");
    m_spec.AddArray<char>("glCode");
    m_spec.AddArray<char>("vkCode");
}

void ShaderAssetLoader::CleanupUnusedMemory()
{
}

std::string& ShaderAssetLoader::GetAssetType()
{
    return shaderAssetTypeName;
}

IAsset *ShaderAssetLoader::CreateEmpty(UniqueId id)
{
    return new ShaderAsset(this, id);
}

void ShaderAssetLoader::Preload(IAsset *asset, LoadMemoryPointer header)
{
    ShaderAsset *sasset = static_cast<ShaderAsset *>(asset);
    FileSpecImporter importer(m_spec, header);
    LocalMemoryAllocator& assetMem = XEngineInstance->GetAssetManager()->GetAssetMemory();

    GraphicsIdentity id = XEngineInstance->GetInterface<DisplayInterface>(HardwareInterfaceType::Display)
        ->GetGraphicsContext()->GetIdentity();

    sasset->SetStage(importer.Get<ShaderStageBit>("stage"));
    if (id == GraphicsIdentity::OpenGL4_6)
    {
        sasset->SetShaderCodeMode(ShaderAssetCode::GlSpirv);
        sasset->UploadCode(importer.GetArray<char>("glCode"), importer.GetArraySize("glCode"));
        sasset->m_vkCodeLength = importer.GetArraySize("vkCode");
        sasset->m_vkCode = assetMem.RequestSpace(sasset->m_vkCodeLength);

        PinnedLocalMemory<char> vkCode = assetMem.GetMemory<char>(sasset->m_vkCode);
        std::memcpy(vkCode.GetData(), importer.GetArray<char>("vkCode"), vkCode.GetSize());
    }
    else if (id == GraphicsIdentity::Vulkan1_2)
    {
        sasset->SetShaderCodeMode(ShaderAssetCode::VkSpirv);
        sasset->UploadCode(importer.GetArray<char>("vkCode"), importer.GetArraySize("vkCode"));
        sasset->m_glCodeLength = importer.GetArraySize("glCode");
        sasset->m_glCode = assetMem.RequestSpace(sasset->m_glCodeLength);

        PinnedLocalMemory<char> glCode = assetMem.GetMemory<char>(sasset->m_glCode);
        std::memcpy(glCode.GetData(), importer.GetArray<char>("glCode"), glCode.GetSize());
    }

    std::vector<std::string> flagKeys;
    std::vector<uint32_t> flagArraySizes;
    std::vector<std::pair<uint32_t, uint32_t>> flagFlattenedValues;

    importer.GetStringArray("flagKeys", flagKeys);

    importer.CopyArray("flagArraySizes", flagArraySizes);
    importer.CopyArray("flagFlattenedValues", flagFlattenedValues);

    sasset->m_flags.reserve(flagArraySizes.size());

    int32_t j = 0;
    for (int32_t i = 0; i < flagKeys.size(); ++i)
    {
        std::vector<std::pair<uint32_t, uint32_t>>& flagVec = sasset->m_flags[flagKeys[i]];
        flagVec.resize(flagArraySizes[i]);
        std::memcpy(flagVec.data(), flagFlattenedValues.data() + j, sizeof(std::pair<uint32_t, uint32_t>) * flagVec.size());
        j += flagVec.size();
    }
}

bool ShaderAssetLoader::CanLoad(IAsset *asset, LoadMemoryPointer loadData)
{
    return true;
}

std::vector<AssetLoadRange> ShaderAssetLoader::Load(IAsset *asset, LoadMemoryPointer loadData)
{
    return {};
}

void ShaderAssetLoader::FinishLoad(IAsset *asset, std::vector<AssetLoadRange>& ranges, std::vector<LoadMemoryPointer>& content, LoadMemoryPointer loadData)
{
}

void ShaderAssetLoader::Copy(IAsset *src, IAsset *dest)
{
    LocalMemoryAllocator& assetMem = XEngineInstance->GetAssetManager()->GetAssetMemory();

    ShaderAsset *sSrc = static_cast<ShaderAsset *>(src);
    ShaderAsset *sDest = static_cast<ShaderAsset *>(dest);

    sDest->m_mode = sSrc->m_mode;
    sDest->m_stage = sSrc->m_stage;
    sDest->m_flags = sSrc->m_flags;

    if (sDest->m_mode == ShaderAssetCode::GlSpirv)
    {
        PinnedLocalMemory<char> srcVk = assetMem.GetMemory<char>(sSrc->m_vkCode);
        sDest->m_vkCode = assetMem.RequestSpace(srcVk.GetSize());

        PinnedLocalMemory<char> destVk = assetMem.GetMemory<char>(sDest->m_vkCode);
        std::memcpy(destVk.GetData(), srcVk.GetData(), destVk.GetSize());

        PinnedLocalMemory<char> srcGl = assetMem.GetMemory<char>(sSrc->m_glCode);
        sDest->UploadCode(srcGl.GetData(), srcGl.GetSize());
    }
    else if (sDest->m_mode == ShaderAssetCode::VkSpirv)
    {
        PinnedLocalMemory<char> srcGl = assetMem.GetMemory<char>(sSrc->m_glCode);
        sDest->m_glCode = assetMem.RequestSpace(srcGl.GetSize());

        PinnedLocalMemory<char> destGl = assetMem.GetMemory<char>(sDest->m_glCode);
        std::memcpy(destGl.GetData(), srcGl.GetData(), destGl.GetSize());

        PinnedLocalMemory<char> srcVk = assetMem.GetMemory<char>(sSrc->m_vkCode);
        sDest->UploadCode(srcVk.GetData(), srcVk.GetSize());
    }
}

void ShaderAssetLoader::Unload(IAsset *asset)
{
    ShaderAsset *shader = static_cast<ShaderAsset *>(asset);

    if (shader->m_mode == ShaderAssetCode::GlSpirv)
    {
        GLShader *sh = static_cast<GLShader *>(shader->m_shader);
        sh->ClearSpecializations();
    }
}

void ShaderAssetLoader::Dispose(IAsset *asset)
{
    ShaderAsset *shader = static_cast<ShaderAsset *>(asset);
    LocalMemoryAllocator& alloc = XEngineInstance->GetAssetManager()->GetAssetMemory();

    if (shader->m_shader)
        delete shader->m_shader;
    if (shader->m_shaderCode)
        delete shader->m_shaderCode;
    if (shader->m_glCode)
        alloc.FreeSpace(shader->m_glCode);
    if (shader->m_vkCode)
        alloc.FreeSpace(shader->m_vkCode);
}

void ShaderAssetLoader::Export(IAsset *asset, AssetDescriptorPreHeader& preHeader, LoadMemoryPointer& header, LoadMemoryPointer& content,
    std::vector<AssetLoadRange>& ranges)
{
    ShaderAsset *shader = static_cast<ShaderAsset *>(asset);
    LocalMemoryAllocator& alloc = XEngineInstance->GetAssetManager()->GetAssetMemory();

    FileSpecExporter exporter(m_spec);

    std::vector<std::string> flagKeys(shader->m_flags.size());
    std::vector<uint32_t> flagArraySizes(shader->m_flags.size());
    std::vector<std::pair<uint32_t, uint32_t>> flagFlattenedValues;

    PinnedLocalMemory<char> glCode = alloc.GetMemory<char>(shader->m_glCode);
    PinnedLocalMemory<char> vkCode = alloc.GetMemory<char>(shader->m_vkCode);


    for (auto [fKey, fValue] : shader->m_flags)
    {
        flagKeys.push_back(fKey);
        flagArraySizes.push_back(fValue.size());
        flagFlattenedValues.insert(flagFlattenedValues.begin(), fValue.begin(), fValue.end());
    }

    exporter.SetArraySize("flagKeys", flagKeys.size());
    exporter.SetArraySize("flagArraySizes", flagArraySizes.size());
    exporter.SetArraySize("flagFlattenedValues", flagFlattenedValues.size());
    exporter.SetArraySize("glCode", glCode.GetSize());
    exporter.SetArraySize("vkCode", vkCode.GetSize());

    header = exporter.AllocateSpace();

    exporter.Set("stage", shader->m_stage);
    exporter.SetArrayData("flagKeys", flagKeys.data());
    exporter.SetArrayData("flagArraySizes", flagArraySizes.data());
    exporter.SetArrayData("flagFlattenedValues", flagFlattenedValues.data());
    exporter.SetArrayData("glCode", glCode.GetData());
    exporter.SetArrayData("vkCode", vkCode.GetData());

    preHeader.Id = shader->GetId();
    strcpy_s(preHeader.AssetType, 64, shaderAssetTypeName.c_str());
    preHeader.HeaderSize = exporter.GetTotalByteSize();
    preHeader.AssetSize = 0;

    content = alloc.RequestSpace(0);
}

void ShaderAssetLoader::AddIncludePath(std::string folder)
{
}

std::string ShaderAssetLoader::ResolveIncludedPath(std::string path)
{
    return std::string();
}
