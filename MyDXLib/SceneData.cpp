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

void SceneData::LoadFromFile(const std::string &sceneDir, const std::string &filename)
{
    // Had troubles with enabling <filesystem> include in MSVC,
    // this is a workaround
    Assimp::Importer importer;
    const aiScene   *scene = importer.ReadFile(
        sceneDir + filename, aiProcessPreset_TargetRealtime_Quality | aiProcess_TransformUVCoords | aiProcess_FlipUVs);

    if (!scene)
        throw std::exception("Couldn't read scene from file");

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    aiString                                               texturePath;

    m_Materials.resize(scene->mNumMaterials);
    for (size_t i = 0; i < scene->mNumMaterials; ++i)
    {
        auto material = scene->mMaterials[i];
#define E(x)                                                                                                           \
    if (material->GetTextureCount(aiTextureType_##x) > 0)                                                              \
    {                                                                                                                  \
        if (material->GetTexture(aiTextureType_##x, 0, &texturePath) == aiReturn_SUCCESS)                              \
        {                                                                                                              \
            m_Materials[i].TexturePaths[TEXTURE_TYPE_##x]                                                              \
                = converter.from_bytes((sceneDir + texturePath.C_Str()).c_str());                                      \
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
                throw std::exception("Weird face");
            for (size_t k = 0; k < face.mNumIndices; ++k)
                indices.push_back(face.mIndices[k]);
        }

        m_Meshes[i].InitData(vertices.data(), vertices.size(), indices.data(), indices.size());
        m_Meshes[i].m_MaterialIndex = mesh->mMaterialIndex;
    }
}
