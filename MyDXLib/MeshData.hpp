#pragma once

#include "pch.hpp"

template <typename V, typename I> class MeshData
{
  public:
    std::vector<V> vertices;
    std::vector<I> indices;

    MeshData()                            = default;
    MeshData(MeshData const &)            = default;
    MeshData(MeshData &&)                 = default;
    MeshData &operator=(MeshData const &) = default;
    MeshData &operator=(MeshData &&)      = default;

    void InitData(V const *vertexData, size_t nVertices, I const *indexData, size_t nIndices)
    {
        vertices = std::vector(vertexData, vertexData + nVertices);
        indices  = std::vector(indexData, indexData + nIndices);
    }

    constexpr size_t VertexSize() const noexcept { return sizeof(V) * vertices.size(); }
    constexpr size_t IndexSize() const noexcept { return sizeof(I) * indices.size(); }

    // void LoadFromFile(const char *path);
};
