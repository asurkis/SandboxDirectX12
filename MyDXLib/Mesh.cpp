#include "Mesh.hpp"

std::pair<PResource, PResource> Mesh::QueryInit(PGraphicsCommandList commandList, MeshData const &data)
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

    PDevice device;
    Assert(commandList->GetDevice(IID_PPV_ARGS(&device)));

    m_VertexCount = data.VertexCount();
    m_IndexCount  = data.IndexCount();

    auto [outVertices, immVertices] = QueryUploadBuffer(commandList, data.VertexBufferStart(), data.VertexBufferSize());
    PResource outIndices, immIndices;
    if (m_UseIndex)
    {
        auto [outIndices1, immIndices1]
            = QueryUploadBuffer(commandList, data.IndexBufferStart(), data.IndexBufferSize());
        outIndices = std::move(outIndices1);
        immIndices = std::move(immIndices1);
    }

    m_VertexBuffer = std::move(outVertices);
    m_IndexBuffer  = std::move(outIndices);

    m_VertexBufferView.BufferLocation = m_VertexBuffer->GetGPUVirtualAddress();
    m_VertexBufferView.SizeInBytes    = static_cast<UINT>(data.VertexBufferSize());
    m_VertexBufferView.StrideInBytes  = data.SingleVertexSize();

    if (m_UseIndex)
        m_IndexBufferView.BufferLocation = m_IndexBuffer->GetGPUVirtualAddress();
    m_IndexBufferView.SizeInBytes = static_cast<UINT>(data.IndexBufferSize());
    m_IndexBufferView.Format      = indexFormat;

    return std::make_pair(std::move(immVertices), std::move(immIndices));
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
