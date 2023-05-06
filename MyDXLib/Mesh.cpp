#include "Mesh.hpp"

std::pair<PResource, PResource> Mesh::QueryInit(PGraphicsCommandList commandList, MeshData const &data)
{
    DXGI_FORMAT indexFormat = DXGI_FORMAT_UNKNOWN;
    switch (data.SingleIndexSize())
    {
    case sizeof(uint16_t): indexFormat = DXGI_FORMAT_R16_UINT; break;
    case sizeof(uint32_t): indexFormat = DXGI_FORMAT_R32_UINT; break;
    default: throw std::exception("Invalid index format");
    }

    PDevice device;
    Assert(commandList->GetDevice(IID_PPV_ARGS(&device)));

    m_VertexCount = data.VertexBufferSize();
    m_IndexCount  = data.IndexBufferSize();

    auto [outVertices, immVertices] = QueryUploadBuffer(commandList, data.VertexBufferStart(), data.VertexBufferSize());
    auto [outIndices, immIndices]   = QueryUploadBuffer(commandList, data.IndexBufferStart(), data.IndexBufferSize());

    m_VertexBuffer = std::move(outVertices);
    m_IndexBuffer  = std::move(outIndices);

    m_VertexBufferView.BufferLocation = m_VertexBuffer->GetGPUVirtualAddress();
    m_VertexBufferView.SizeInBytes    = static_cast<UINT>(data.VertexBufferSize());
    m_VertexBufferView.StrideInBytes  = data.SingleVertexSize();

    m_IndexBufferView.BufferLocation = m_IndexBuffer->GetGPUVirtualAddress();
    m_IndexBufferView.SizeInBytes    = static_cast<UINT>(data.IndexBufferSize());
    m_IndexBufferView.Format         = indexFormat;

    return std::make_pair(std::move(immVertices), std::move(immIndices));
}

void Mesh::Draw(PGraphicsCommandList commandList)
{
    commandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
    commandList->IASetIndexBuffer(&m_IndexBufferView);
    commandList->DrawIndexedInstanced(static_cast<UINT>(m_IndexCount), 1, 0, 0, 0);
}
