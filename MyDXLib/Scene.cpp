#include "Scene.hpp"
#include "SceneData.hpp"
#include "Utils.hpp"

void Material::QueryInit(ResourceUploadBatch &rub, const MaterialData &data) {}

void Material::Draw(PGraphicsCommandList commandList) {}

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

void Mesh::Draw(PGraphicsCommandList commandList)
{
    commandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
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

void Scene::QueryInit(PDevice device, ResourceUploadBatch &rub, const SceneData &data)
{
    auto &&materialData = data.GetMaterials();
    auto &&meshData     = data.GetMeshes();

    m_Materials.resize(materialData.size());
    m_Meshes.resize(meshData.size());

    for (std::size_t i = 0; i < materialData.size(); ++i) {}

    for (std::size_t i = 0; i < meshData.size(); ++i)
        m_Meshes[i].QueryInit(device, rub, meshData[i], &m_Materials[meshData[i].m_MaterialIndex]);
}

void Scene::Draw(PGraphicsCommandList commandList)
{
    for (auto &&mesh : m_Meshes)
        mesh.Draw(commandList);
}
