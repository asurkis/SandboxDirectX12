#include "SceneData.hpp"
#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

struct PosUV
{
    aiVector3D pos;
    aiVector2D uv;
};

void ObjectData::ParseNode(const aiNode *node)
{
    m_Transform(0, 0) = node->mTransformation.a1;
    m_Transform(0, 1) = node->mTransformation.a2;
    m_Transform(0, 2) = node->mTransformation.a3;
    m_Transform(0, 3) = node->mTransformation.a4;
    m_Transform(1, 0) = node->mTransformation.b1;
    m_Transform(1, 1) = node->mTransformation.b2;
    m_Transform(1, 2) = node->mTransformation.b3;
    m_Transform(1, 3) = node->mTransformation.b4;
    m_Transform(2, 0) = node->mTransformation.c1;
    m_Transform(2, 1) = node->mTransformation.c2;
    m_Transform(2, 2) = node->mTransformation.c3;
    m_Transform(2, 3) = node->mTransformation.c4;
    m_Transform(3, 0) = node->mTransformation.d1;
    m_Transform(3, 1) = node->mTransformation.d2;
    m_Transform(3, 2) = node->mTransformation.d3;
    m_Transform(3, 3) = node->mTransformation.d4;

    m_MeshIdx.resize(node->mNumMeshes);
    for (size_t i = 0; i < node->mNumMeshes; ++i)
        m_MeshIdx[i] = node->mMeshes[i];

    m_Children.resize(node->mNumChildren);
    for (size_t i = 0; i < node->mNumChildren; ++i)
    {
        m_Children[i] = std::make_unique<ObjectData>();
        m_Children[i]->ParseNode(node->mChildren[i]);
    }
}

void SceneData::LoadFromFile(const std::filesystem::path &scenePath)
{
    // Had troubles with enabling <filesystem> include in MSVC,
    // this is a workaround
    Assimp::Importer      importer;
    std::filesystem::path pathComponents = scenePath;
    auto                  sceneDirW      = pathComponents.remove_filename().generic_wstring();

    const aiScene *scene = importer.ReadFile(scenePath.generic_string(),
                                             aiProcessPreset_TargetRealtime_Quality | aiProcess_TransformUVCoords
                                                 | aiProcess_FlipUVs | aiProcess_ValidateDataStructure);

    if (!scene)
        throw std::exception("Couldn't read scene from file");

    std::wstring_convert<std::codecvt<wchar_t, char, std::mbstate_t>> converter;
    aiString                                                          texturePath;

    m_Materials.resize(scene->mNumMaterials);
    for (size_t i = 0; i < scene->mNumMaterials; ++i)
    {
        auto material = scene->mMaterials[i];
#define E(x)                                                                                                           \
    if (material->GetTextureCount(aiTextureType_##x) > 0)                                                              \
    {                                                                                                                  \
        if (material->GetTexture(aiTextureType_##x, 0, &texturePath) == aiReturn_SUCCESS)                              \
        {                                                                                                              \
            m_Materials[i].TexturePaths[TEXTURE_TYPE_##x] = sceneDirW + converter.from_bytes(texturePath.C_Str());     \
        }                                                                                                              \
    }
        ENUM_TEXTURE_TYPES
#undef E
    }

    m_Meshes.resize(scene->mNumMeshes);
    for (size_t i = 0; i < scene->mNumMeshes; ++i)
    {
        auto mesh = scene->mMeshes[i];

        if (mesh->mNumUVComponents[0] < 2)
            throw std::exception("Mesh doesn't contain UV coordinates");

        std::vector<PosUV>    vertices;
        std::vector<uint32_t> indices;

        vertices.reserve(mesh->mNumVertices);
        indices.reserve(size_t(3) * mesh->mNumFaces);

        for (size_t j = 0; j < mesh->mNumVertices; ++j)
        {
            auto       vert = mesh->mVertices[j];
            aiVector3D uvw  = mesh->mTextureCoords[0][j];
            vertices.push_back(PosUV{
                vert, {uvw.x, uvw.y}
            });
        }

        for (size_t j = 0; j < mesh->mNumFaces; ++j)
        {
            auto &&face = mesh->mFaces[j];
            if (face.mNumIndices != 3)
                // throw std::exception("Weird face");
                continue;
            for (size_t k = 0; k < face.mNumIndices; ++k)
                indices.push_back(face.mIndices[k]);
        }

        m_Meshes[i].InitData(vertices.data(), vertices.size(), indices.data(), indices.size());
        m_Meshes[i].m_MaterialIndex = mesh->mMaterialIndex;
    }

    m_RootObject.ParseNode(scene->mRootNode);
}
