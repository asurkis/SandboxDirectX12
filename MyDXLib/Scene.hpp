#pragma once

#include "pch.hpp"

#include "SceneData.hpp"

class Texture
{
    DescriptorHeap &m_DescriptorHeap;
    PResource       m_Data;
    size_t          m_DescriptorId;

  public:
    Texture(PDevice                  device,
            ResourceUploadBatch     &rub,
            DirectX::DescriptorHeap &heap,
            const wchar_t           *path,
            size_t                   descriptorId);

    void Draw(PGraphicsCommandList commandList) const;
};

class Material
{
    Texture *m_Textures[TEXTURE_TYPE_COUNT] = {};

  public:
    Material(PDevice                  device,
             ResourceUploadBatch     &rub,
             DirectX::DescriptorHeap &heap,
             const MaterialData      &data,
             size_t                  &takenId);

    void Draw(PGraphicsCommandList commandList) const;
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
    void Draw(PGraphicsCommandList commandList) const;
};

class SceneObject
{
    std::vector<std::unique_ptr<SceneObject>> m_Children;
    std::vector<const Mesh *>                 m_Meshes;
    DirectX::XMMATRIX                         m_Transform;

  public:
    void QueryInit(const ObjectData &data, const std::vector<Mesh> &meshes);
    void Draw(PGraphicsCommandList commandList, const DirectX::XMMATRIX &mvpMatrix) const;
};

class Scene
{
    std::vector<Material> m_Materials;
    std::vector<Mesh>     m_Meshes;
    SceneObject           m_Root;

    DescriptorHeap *m_DescriptorHeap;

  public:
    void QueryInit(PDevice device, ResourceUploadBatch &rub, DescriptorHeap &descriptorHeap, const SceneData &data);
    void Draw(PGraphicsCommandList commandList, const DirectX::XMMATRIX &mvpMatrix) const;
};
