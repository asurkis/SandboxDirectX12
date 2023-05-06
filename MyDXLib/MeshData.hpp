#pragma once

#include "pch.hpp"

class MeshData
{
    std::vector<std::byte> m_VertexBuffer;
    std::vector<std::byte> m_IndexBuffer;
    size_t                 m_VertexSize;
    size_t                 m_IndexSize;

  public:
    MeshData()                            = default;
    MeshData(MeshData const &)            = default;
    MeshData(MeshData &&)                 = default;
    MeshData &operator=(MeshData const &) = default;
    MeshData &operator=(MeshData &&)      = default;

    template <typename V, typename I>
    void InitData(V const *vertexData, size_t nVertices, I const *indexData, size_t nIndices)
    {
        std::byte const *pVertices = static_cast<std::byte const *>(static_cast<void const *>(vertexData));
        std::byte const *pIndices  = static_cast<std::byte const *>(static_cast<void const *>(indexData));
        m_VertexSize               = sizeof(V);
        m_IndexSize                = sizeof(I);
        m_VertexBuffer             = std::vector(pVertices, pVertices + sizeof(V) * nVertices);
        m_IndexBuffer              = std::vector(pIndices, pIndices + sizeof(I) * nIndices);
    }

    const void      *VertexBufferStart() const noexcept { return m_VertexBuffer.data(); }
    const void      *IndexBufferStart() const noexcept { return m_IndexBuffer.data(); }
    size_t           VertexBufferSize() const noexcept { return m_VertexBuffer.size(); }
    size_t           IndexBufferSize() const noexcept { return m_IndexBuffer.size(); }
    constexpr size_t SingleVertexSize() const noexcept { return m_VertexSize; }
    constexpr size_t SingleIndexSize() const noexcept { return m_IndexSize; }
    size_t           VertexCount() const noexcept { return VertexBufferSize() / SingleVertexSize(); }
    size_t           IndexCount() const noexcept { return IndexBufferSize() / SingleIndexSize(); }

    void LoadFromFile(const char *path);
};
