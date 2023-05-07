#pragma once

#include "pch.hpp"

class MeshData
{
    std::vector<char> m_VertexBuffer;
    std::vector<char> m_IndexBuffer;
    size_t            m_VertexCount = 0;
    size_t            m_IndexCount  = 0;
    size_t            m_VertexSize  = 0;
    size_t            m_IndexSize   = 0;

  public:
    MeshData()                            = default;
    MeshData(MeshData const &)            = default;
    MeshData(MeshData &&)                 = default;
    MeshData &operator=(MeshData const &) = default;
    MeshData &operator=(MeshData &&)      = default;

    template <typename V, typename I>
    void InitData(V const *vertexData, size_t nVertices, I const *indexData, size_t nIndices)
    {
        char const *pVertices = static_cast<char const *>(static_cast<void const *>(vertexData));
        char const *pIndices  = static_cast<char const *>(static_cast<void const *>(indexData));
        m_VertexCount         = nVertices;
        m_VertexSize          = sizeof(V);
        m_IndexCount          = nIndices;
        m_IndexSize           = sizeof(I);
        m_VertexBuffer        = std::vector(pVertices, pVertices + sizeof(V) * nVertices);
        m_IndexBuffer         = std::vector(pIndices, pIndices + sizeof(I) * nIndices);
    }

    template <typename V> void InitVertices(V const *vertexData, size_t nVertices)
    {
        char const *pVertices = static_cast<char const *>(static_cast<void const *>(vertexData));
        m_VertexCount         = nVertices;
        m_VertexSize          = sizeof(V);
        m_IndexCount          = 0;
        m_IndexSize           = 0;
        m_VertexBuffer        = std::vector(pVertices, pVertices + sizeof(V) * nVertices);
        m_IndexBuffer.clear();
    }

    const void      *VertexBufferStart() const noexcept { return m_VertexBuffer.data(); }
    const void      *IndexBufferStart() const noexcept { return m_IndexBuffer.data(); }
    size_t           VertexBufferSize() const noexcept { return m_VertexBuffer.size(); }
    size_t           IndexBufferSize() const noexcept { return m_IndexBuffer.size(); }
    constexpr size_t SingleVertexSize() const noexcept { return m_VertexSize; }
    constexpr size_t SingleIndexSize() const noexcept { return m_IndexSize; }
    constexpr size_t VertexCount() const noexcept { return m_VertexCount; }
    constexpr size_t IndexCount() const noexcept { return m_IndexCount; }

    void LoadFromFile(const char *path);
};
