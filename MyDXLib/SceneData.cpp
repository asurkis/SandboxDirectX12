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
}
