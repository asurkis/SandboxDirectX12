#pragma once

#include "pch.hpp"

#define ENUM_TEXTURE_TYPES                                                                                             \
    E(BASE_COLOR) E(NORMAL_CAMERA) E(EMISSION_COLOR) E(METALNESS) E(DIFFUSE_ROUGHNESS) E(AMBIENT_OCCLUSION)

enum TextureType
{
#define E(x) TEXTURE_TYPE_##x,
    ENUM_TEXTURE_TYPES
#undef E
        TEXTURE_TYPE_COUNT
};

class MaterialData
{
  public:
    std::wstring TexturePaths[TEXTURE_TYPE_COUNT];
};

class MeshData
{
    std::vector<char> m_VertexBuffer;
    std::vector<char> m_IndexBuffer;
    size_t            m_VertexCount = 0;
    size_t            m_IndexCount  = 0;
    size_t            m_VertexSize  = 0;
    size_t            m_IndexSize   = 0;

  public:
    size_t m_MaterialIndex = 0;

    MeshData()                            = default;
    MeshData(MeshData const &)            = default;
    MeshData(MeshData &&)                 = default;
    MeshData &operator=(MeshData const &) = default;
    MeshData &operator=(MeshData &&)      = default;
    virtual ~MeshData() {}

    template <typename V, typename I>
    void InitData(const V *vertexData, size_t nVertices, const I *indexData, size_t nIndices)
    {
        char const *pVertices = static_cast<const char *>(static_cast<const void *>(vertexData));
        char const *pIndices  = static_cast<const char *>(static_cast<const void *>(indexData));
        m_VertexCount         = nVertices;
        m_VertexSize          = sizeof(V);
        m_IndexCount          = nIndices;
        m_IndexSize           = sizeof(I);
        m_VertexBuffer        = std::vector(pVertices, pVertices + sizeof(V) * nVertices);
        m_IndexBuffer         = std::vector(pIndices, pIndices + sizeof(I) * nIndices);
    }

    template <typename V> void InitVertices(const V *vertexData, size_t nVertices)
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
};

class MeshMaterialData : public MeshData
{
  public:
    MeshMaterialData()                                    = default;
    MeshMaterialData(MeshMaterialData const &)            = default;
    MeshMaterialData(MeshMaterialData &&)                 = default;
    MeshMaterialData &operator=(MeshMaterialData const &) = default;
    MeshMaterialData &operator=(MeshMaterialData &&)      = default;
};

class SceneData
{
    std::vector<MaterialData>     m_Materials;
    std::vector<MeshMaterialData> m_Meshes;

  public:
    const std::vector<MaterialData>     &GetMaterials() const noexcept { return m_Materials; }
    const std::vector<MeshMaterialData> &GetMeshes() const noexcept { return m_Meshes; }

    void LoadFromFile(const std::string &sceneDir, const std::string &filename);
};
