#include "Scene.hpp"
#include "SceneData.hpp"
#include "Utils.hpp"

using namespace DirectX;

Material::Material(
    PDevice device, ResourceUploadBatch &rub, DescriptorHeap &heap, const MaterialData &data, size_t &takenId)
    : m_DescriptorHeap(heap)
{
    for (size_t i = 0; i < TEXTURE_TYPE_COUNT; ++i)
        m_DescriptorIdx[i] = ~0;

    for (size_t i = 0; i < TEXTURE_TYPE_COUNT; ++i)
    {
        size_t id          = takenId++;
        m_DescriptorIdx[i] = id;

        if (data.TexturePaths[i].empty())
            continue;

        Assert(CreateWICTextureFromFile(
            device.Get(), rub, data.TexturePaths[i].c_str(), m_Textures[i].ReleaseAndGetAddressOf()));

        D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
        desc.Format                          = DXGI_FORMAT_UNKNOWN;
        desc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURE2D;
        desc.Shader4ComponentMapping         = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        desc.Texture2D.MostDetailedMip       = 0;
        desc.Texture2D.MipLevels             = -1;
        desc.Texture2D.PlaneSlice            = 0;
        desc.Texture2D.ResourceMinLODClamp   = 0.0f;
        device->CreateShaderResourceView(m_Textures[i].Get(), &desc, m_DescriptorHeap.GetCpuHandle(id));
    }
}

void Material::Draw(PGraphicsCommandList commandList) const
{
    if (!m_Textures[0])
        return;
    commandList->SetGraphicsRootDescriptorTable(1, m_DescriptorHeap.GetGpuHandle(m_DescriptorIdx[0]));
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

void SceneObject::Draw(PGraphicsCommandList commandList, const XMMATRIX &mvpMatrix) const
{
    XMMATRIX accMatrix = m_Transform * mvpMatrix;
    commandList->SetGraphicsRoot32BitConstants(0, 16, &accMatrix, 0);

    for (const Mesh *mesh : m_Meshes)
        mesh->Draw(commandList);

    for (auto &child : m_Children)
        child->Draw(commandList, accMatrix);
}

void Scene::QueryInit(PDevice device, ResourceUploadBatch &rub, DescriptorHeap &descriptorHeap, const SceneData &data)
{
    auto &&materialData = data.GetMaterials();
    auto &&meshData     = data.GetMeshes();

    m_Materials.clear();
    m_Materials.reserve(materialData.size());
    m_Meshes.resize(meshData.size());

    size_t takenId = 1;
    for (std::size_t i = 0; i < materialData.size(); ++i)
        m_Materials.emplace_back(device, rub, descriptorHeap, materialData[i], takenId);

    for (std::size_t i = 0; i < meshData.size(); ++i)
    {
        Material *material = nullptr;
        if (meshData[i].m_MaterialIndex < m_Materials.size())
            material = &m_Materials[meshData[i].m_MaterialIndex];
        m_Meshes[i].QueryInit(device, rub, meshData[i], material);
    }

    m_Root.QueryInit(data.GetRoot(), m_Meshes);
}

void Scene::Draw(PGraphicsCommandList commandList, const DirectX::XMMATRIX &mvpMatrix) const
{
    m_Root.Draw(commandList, mvpMatrix);
    // for (auto &&mesh : m_Meshes)
    //     mesh.Draw(commandList);
}
