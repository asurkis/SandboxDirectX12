#pragma once

#include "pch.hpp"

class MeshData;
class SceneData;

class Mesh
{
    PResource                m_VertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView = {};
    PResource                m_IndexBuffer;
    D3D12_INDEX_BUFFER_VIEW  m_IndexBufferView = {};

    size_t m_VertexCount = 0;
    size_t m_IndexCount  = 0;
    bool   m_UseIndex    = false;

  public:
    const D3D12_VERTEX_BUFFER_VIEW &GetVertexView() const noexcept { return m_VertexBufferView; }
    const D3D12_INDEX_BUFFER_VIEW  &GetIndexView() const noexcept { return m_IndexBufferView; }

    std::pair<PResource, PResource> QueryInit(PGraphicsCommandList commandList, const MeshData &data);
    void                            Draw(PGraphicsCommandList commandList);
};

class Scene
{
    std::vector<Mesh> m_Meshes;

  public:
    std::vector<PResource> QueryInit(PGraphicsCommandList commandList, const SceneData &data);
    void                   Draw(PGraphicsCommandList commandList);
};
