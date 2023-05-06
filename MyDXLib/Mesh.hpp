#pragma once

#include "pch.hpp"

#include "MeshData.hpp"
#include "Utils.hpp"

class Mesh
{
    PResource                m_VertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
    PResource                m_IndexBuffer;
    D3D12_INDEX_BUFFER_VIEW  m_IndexBufferView;

    size_t m_VertexCount;
    size_t m_IndexCount;

  public:
    const D3D12_VERTEX_BUFFER_VIEW &GetVertexView() const noexcept { return m_VertexBufferView; }
    const D3D12_INDEX_BUFFER_VIEW  &GetIndexView() const noexcept { return m_IndexBufferView; }

    std::pair<PResource, PResource> QueryInit(PGraphicsCommandList commandList, MeshData const &data);
    void Draw(PGraphicsCommandList commandList);
};
