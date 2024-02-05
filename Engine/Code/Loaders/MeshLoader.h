#pragma once

#include <filesystem>

#include <assimp/scene.h>

#include <Level/Entity.h>

class MeshLoader
{
public:
    static Entity Load(const std::filesystem::path& path);

private:
    struct MeshletData
    {
        std::vector<Meshlet> meshlets;
        std::vector<uint32_t> meshletVertices;
        std::vector<uint32_t> meshletTriangles;
    };

private:
    static TexturePtr LoadMaterialTexture(aiMaterial* assetMaterial, aiTextureType textureType, std::string_view sceneFolder, CommandBufferPtr& cmdBuffer);
    static std::vector<MaterialPtr> RetrieveMaterials(const aiScene* assetScene, std::string_view sceneFolder, std::string_view sceneName);
    static std::vector<uint32_t> RetrieveIndices(const aiMesh* assetMesh);
    static std::vector<float> RetrievePositions(const aiMesh* assetMesh);
    static std::vector<int16_t> RetrieveNormals(const aiMesh* assetMesh);
    static std::vector<int16_t> RetrieveTangentsBitangents(const aiMesh* assetMesh);
    static std::vector<int16_t> RetrieveUV0(const aiMesh* assetMesh);
    static std::vector<int16_t> RetrieveColors(const aiMesh* assetMesh);
    static MeshletData BuildMeshlets(const std::vector<float>& positions, size_t vtxCount, const std::vector<uint32_t>& indices);
    static std::vector<MeshPtr> RetrieveMeshes(const aiScene* assetScene, std::string_view sceneName);
    static void PopulateNode(const aiScene* assetScene, const aiNode* assetNode, std::string_view sceneName, Entity node, const glm::mat4& parentTransform, std::vector<MaterialPtr>& materials, std::vector<MeshPtr>& meshes);
    static Entity RetrieveNodes(const aiScene* assetScene, std::vector<MaterialPtr>& materials, std::vector<MeshPtr>& meshes, std::string_view sceneName);
};