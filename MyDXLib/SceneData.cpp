#include "SceneData.hpp"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

struct PosUV
{
    aiVector3D pos;
    aiVector2D uv;
};

std::vector<MeshData> MeshData::LoadFromFile(const char *path)
{
    unsigned int importFlags = aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices | aiProcess_Triangulate
                             | aiProcess_PreTransformVertices | aiProcess_ValidateDataStructure
                             | aiProcess_ImproveCacheLocality;

    Assimp::Importer importer;

    const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate);

    if (!scene)
        throw std::exception("Couldn't read scene from file");

    std::vector<MeshData> result(scene->mNumMeshes);

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

        result[i].InitData(vertices.data(), vertices.size(), indices.data(), indices.size());
    }

    return result;
}
