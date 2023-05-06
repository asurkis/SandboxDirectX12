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
    template <typename V, typename I>
    std::pair<PResource, PResource> QueryInit(PGraphicsCommandList commandList, MeshData<V, I> const &data)
    {
        DXGI_FORMAT indexFormat = DXGI_FORMAT_UNKNOWN;
        switch (sizeof(I))
        {
        case sizeof(uint16_t): indexFormat = DXGI_FORMAT_R16_UINT; break;
        case sizeof(uint32_t): indexFormat = DXGI_FORMAT_R32_UINT; break;
        default: throw std::exception("Invalid index format");
        }

        PDevice device;
        Assert(commandList->GetDevice(IID_PPV_ARGS(&device)));

        m_VertexCount = data.vertices.size();
        m_IndexCount  = data.indices.size();

        auto [outVertices, immVertices] = QueryUploadBuffer(commandList, data.vertices.data(), data.VertexSize());
        auto [outIndices, immIndices]   = QueryUploadBuffer(commandList, data.indices.data(), data.IndexSize());

        m_VertexBuffer = std::move(outVertices);
        m_IndexBuffer  = std::move(outIndices);

        m_VertexBufferView.BufferLocation = m_VertexBuffer->GetGPUVirtualAddress();
        m_VertexBufferView.SizeInBytes    = static_cast<UINT>(data.VertexSize());
        m_VertexBufferView.StrideInBytes  = sizeof(V);

        m_IndexBufferView.BufferLocation = m_IndexBuffer->GetGPUVirtualAddress();
        m_IndexBufferView.SizeInBytes    = static_cast<UINT>(data.IndexSize());
        m_IndexBufferView.Format         = indexFormat;

        return std::make_pair(std::move(immVertices), std::move(immIndices));
    }

    const D3D12_VERTEX_BUFFER_VIEW &GetVertexView() const noexcept { return m_VertexBufferView; }
    const D3D12_INDEX_BUFFER_VIEW  &GetIndexView() const noexcept { return m_IndexBufferView; }

    void Draw(PGraphicsCommandList commandList)
    {
        commandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
        commandList->IASetIndexBuffer(&m_IndexBufferView);
        commandList->DrawIndexedInstanced(static_cast<UINT>(m_IndexCount), 1, 0, 0, 0);
    }
};
