#pragma once

#include "pch.hpp"

#include "SceneData.hpp"

class Material
{
  public:
    void QueryInit(ResourceUploadBatch &rub, const MaterialData &data);
    void Draw(PGraphicsCommandList commandList);
};

class Mesh
{
    PResource                m_VertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView = {};
    PResource                m_IndexBuffer;
    D3D12_INDEX_BUFFER_VIEW  m_IndexBufferView = {};

    const Material *m_Material      = nullptr;
    size_t          m_MaterialIndex = 0;

    size_t m_VertexCount = 0;
    size_t m_IndexCount  = 0;
    bool   m_UseIndex    = false;

  public:
    const D3D12_VERTEX_BUFFER_VIEW &GetVertexView() const noexcept { return m_VertexBufferView; }
    const D3D12_INDEX_BUFFER_VIEW  &GetIndexView() const noexcept { return m_IndexBufferView; }

    void QueryInit(PDevice device, ResourceUploadBatch &rub, const MeshData &data, const Material *material = nullptr);

    void Draw(PGraphicsCommandList commandList);
};

class Scene
{
    std::vector<Material> m_Materials;
    std::vector<Mesh>     m_Meshes;

  public:
    void QueryInit(PDevice device, ResourceUploadBatch &rub, const SceneData &data);
    void Draw(PGraphicsCommandList commandList);
};
