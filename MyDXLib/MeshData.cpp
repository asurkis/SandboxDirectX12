#include "MeshData.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

void MeshData::LoadFromFile(const char *path)
{
    Assimp::Importer importer;
    unsigned int     importFlags = aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices | aiProcess_Triangulate;
    const aiScene   *scene       = importer.ReadFile(path, importFlags);

    if (!scene)
        throw std::exception("Couldn't read scene from file");

    std::vector<float> vertices;
    size_t             totalVertices = 0;

    for (size_t i = 0; i < 1 /* scene->mNumMeshes */; ++i)
        totalVertices += scene->mMeshes[i]->mNumVertices;

    if (totalVertices > UINT32_MAX)
        throw std::exception("Model too large");

    vertices.reserve(3 * totalVertices);
    size_t totalJ = 0;

    for (size_t i = 0; i < 1 /* scene->mNumMeshes */; ++i)
    {
        auto mesh = scene->mMeshes[i];

        if (mesh->mNumUVComponents[0] < 2)
            throw std::exception("Mesh doesn't contain UV coordinates");

        for (size_t j = 0; j < mesh->mNumVertices; ++j)
        {
            auto vert = mesh->mVertices[j];
            vertices.push_back(vert.x);
            vertices.push_back(vert.y);
            vertices.push_back(vert.z);

            vertices.push_back(mesh->mTextureCoords[0][j].x);
            vertices.push_back(mesh->mTextureCoords[0][j].y);
        }
    }

    InitVertices(vertices.data(), vertices.size());
}
