#include "Scene.hpp"
#include "SceneData.hpp"
#include "Utils.hpp"

using namespace DirectX;

Texture::Texture(
    PDevice device, ResourceUploadBatch &rub, DescriptorHeap &heap, const wchar_t *path, size_t descriptorId)
    : m_DescriptorHeap(heap),
      m_DescriptorId(descriptorId)
{
    Assert(CreateWICTextureFromFile(device.Get(), rub, path, m_Data.ReleaseAndGetAddressOf()));

    D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
    desc.Format                          = DXGI_FORMAT_UNKNOWN;
    desc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURE2D;
    desc.Shader4ComponentMapping         = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    desc.Texture2D.MostDetailedMip       = 0;
    desc.Texture2D.MipLevels             = -1;
    desc.Texture2D.PlaneSlice            = 0;
    desc.Texture2D.ResourceMinLODClamp   = 0.0f;
    device->CreateShaderResourceView(m_Data.Get(), &desc, m_DescriptorHeap.GetCpuHandle(m_DescriptorId));
}

void Texture::Draw(PGraphicsCommandList commandList) const
{
    commandList->SetGraphicsRootDescriptorTable(1, m_DescriptorHeap.GetGpuHandle(m_DescriptorId));
}

Material::Material(std::unordered_map<std::wstring_view, Texture *> textures, const MaterialData &data)
{
    for (size_t i = 0; i < TEXTURE_TYPE_COUNT; ++i)
        m_Textures[i] = textures[data.TexturePaths[i]];
}

void Material::Draw(PGraphicsCommandList commandList) const
{
    if (!m_Textures[0])
        return;
    m_Textures[0]->Draw(commandList);
}

void Mesh::QueryInit(PDevice device, ResourceUploadBatch &rub, const MeshData &data, const Material *material)
{
    DXGI_FORMAT indexFormat = DXGI_FORMAT_UNKNOWN;
    switch (data.SingleIndexSize())
    {
    case sizeof(uint16_t): indexFormat = DXGI_FORMAT_R16_UINT; break;
    case sizeof(uint32_t): indexFormat = DXGI_FORMAT_R32_UINT; break;
    default: indexFormat = DXGI_FORMAT_UNKNOWN; break;
    }

    m_UseIndex = data.IndexCount() != 0;
    if (m_UseIndex && indexFormat == DXGI_FORMAT_UNKNOWN)
        throw std::exception("Unknown index format");

    m_VertexCount = data.VertexCount();
    m_IndexCount  = data.IndexCount();

    m_VertexBuffer = QueryUploadBuffer(device, rub, data.VertexBufferStart(), data.VertexBufferSize());
    if (m_UseIndex)
        m_IndexBuffer = QueryUploadBuffer(device, rub, data.IndexBufferStart(), data.IndexBufferSize());

    m_VertexBufferView.BufferLocation = m_VertexBuffer->GetGPUVirtualAddress();
    m_VertexBufferView.SizeInBytes    = static_cast<UINT>(data.VertexBufferSize());
    m_VertexBufferView.StrideInBytes  = data.SingleVertexSize();

    if (m_UseIndex)
        m_IndexBufferView.BufferLocation = m_IndexBuffer->GetGPUVirtualAddress();
    m_IndexBufferView.SizeInBytes = static_cast<UINT>(data.IndexBufferSize());
    m_IndexBufferView.Format      = indexFormat;

    m_Material      = material;
    m_MaterialIndex = data.m_MaterialIndex;
}

void Mesh::Draw(PGraphicsCommandList commandList) const
{
    commandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);

    if (m_Material)
        m_Material->Draw(commandList);

    if (m_UseIndex)
    {
        commandList->IASetIndexBuffer(&m_IndexBufferView);
        commandList->DrawIndexedInstanced(static_cast<UINT>(m_IndexCount), 1, 0, 0, 0);
    }
    else
    {
        commandList->DrawInstanced(m_VertexCount, 1, 0, 0);
    }
}

void SceneObject::QueryInit(const ObjectData &data, const std::vector<Mesh> &meshes)
{
    m_Transform = XMLoadFloat4x4(&data.m_Transform);

    m_Meshes.resize(data.GetMeshIdx().size());
    for (size_t i = 0; i < m_Meshes.size(); ++i)
        m_Meshes[i] = &meshes[data.GetMeshIdx()[i]];

    m_Children.resize(data.GetChildren().size());
    for (size_t i = 0; i < m_Children.size(); ++i)
    {
        m_Children[i] = std::make_unique<SceneObject>();
        m_Children[i]->QueryInit(*(data.GetChildren()[i]), meshes);
    }
}

void SceneObject::Draw(PGraphicsCommandList     commandList,
                       const DirectX::XMMATRIX &model,
                       const DirectX::XMMATRIX &view,
                       const DirectX::XMMATRIX &projection) const
{
    XMMATRIX constants[3];
    constants[0] = m_Transform * model;
    constants[1] = view;
    constants[2] = projection;
    commandList->SetGraphicsRoot32BitConstants(0, 48, &constants, 0);

    for (const Mesh *mesh : m_Meshes)
        mesh->Draw(commandList);

    for (auto &child : m_Children)
        child->Draw(commandList, constants[0], view, projection);
}

void Scene::QueryInit(PDevice device, ResourceUploadBatch &rub, DescriptorHeap &descriptorHeap, const SceneData &data)
{
    auto &&texturePaths = data.GetTexturePaths();
    auto &&materialData = data.GetMaterials();
    auto &&meshData     = data.GetMeshes();

    std::unordered_map<std::wstring_view, Texture *> textureMapping;
    textureMapping[L""] = nullptr;

    m_Textures.clear();
    m_Textures.reserve(texturePaths.size());
    m_Materials.clear();
    m_Materials.reserve(materialData.size());
    m_Meshes.resize(meshData.size());

    size_t takenId = 1;
    for (auto &&path : texturePaths)
    {
        // if (m_Textures.empty())
        // {
        m_Textures.emplace_back(device, rub, descriptorHeap, path.c_str(), takenId++);
        textureMapping[path] = &m_Textures[m_Textures.size() - 1];
        // }
        // else
        // {
        //     textureMapping[path] = &m_Textures[0];
        // }
    }

    for (size_t i = 0; i < materialData.size(); ++i)
        m_Materials.emplace_back(textureMapping, materialData[i]);

    for (size_t i = 0; i < meshData.size(); ++i)
    {
        Material *material = nullptr;
        if (meshData[i].m_MaterialIndex < m_Materials.size())
            material = &m_Materials[meshData[i].m_MaterialIndex];
        m_Meshes[i].QueryInit(device, rub, meshData[i], material);
    }

    m_Root.QueryInit(data.GetRoot(), m_Meshes);
}

void Scene::Draw(PGraphicsCommandList     commandList,
                 const DirectX::XMMATRIX &model,
                 const DirectX::XMMATRIX &view,
                 const DirectX::XMMATRIX &projection) const
{
    m_Root.Draw(commandList, model, view, projection);
    // for (auto &&mesh : m_Meshes)
    //     mesh.Draw(commandList);
}
