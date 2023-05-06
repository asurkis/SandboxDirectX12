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

    for (size_t i = 0; i < scene->mNumMeshes; ++i) {}
}
