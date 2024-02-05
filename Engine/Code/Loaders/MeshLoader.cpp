#include "MeshLoader.h"

#include <assimp/scene.h>
#include <assimp/cimport.h>
#include <assimp/postprocess.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>
#include <meshoptimizer.h>

#include <Application/Application.h>

Entity MeshLoader::Load(const std::filesystem::path& path)
{
    std::string asciiPath = path.string();
    const aiScene* assetScene = aiImportFile(asciiPath.c_str(), aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_FlipUVs);
    if (!assetScene)
    {
        LogError("Scene \'{}\' loading failed: {}", asciiPath, aiGetErrorString());
        return {};
    }

    std::string sceneFolder = path.parent_path().string() + '/';
    std::string sceneName = path.filename().replace_extension("").string();

    std::vector<MaterialPtr> materials = RetrieveMaterials(assetScene, sceneFolder, sceneName);
    std::vector<MeshPtr> meshes = RetrieveMeshes(assetScene, sceneName);
    Entity rootEntity = RetrieveNodes(assetScene, materials, meshes, sceneName);

    aiReleaseImport(assetScene);

    return rootEntity;
}

TexturePtr MeshLoader::LoadMaterialTexture(aiMaterial* assetMaterial, aiTextureType textureType, std::string_view sceneFolder, CommandBufferPtr& cmdBuffer)
{
    ProfileFunction();

    uint32_t uvSet = 0;
    aiString textureName;
    aiReturn ret = assetMaterial->GetTexture(textureType, 0, &textureName, nullptr, &uvSet);

    if (ret != aiReturn_SUCCESS)
    {
        return nullptr;
    }

    std::filesystem::path texturePath = std::string(sceneFolder) + textureName.data;
    TexturePtr texture = Texture::LoadFromFile(texturePath);

    return texture;
}

std::vector<MaterialPtr> MeshLoader::RetrieveMaterials(const aiScene* assetScene, std::string_view sceneFolder, std::string_view sceneName)
{
    ProfileFunction();

    std::vector<MaterialPtr> materials;

    CommandBufferPtr cmdBuffer = Renderer::Get()->GetLoadCmdBuffer();

    for (uint32_t i = 0; i < assetScene->mNumMaterials; i++)
    {
        aiMaterial* assetMaterial = assetScene->mMaterials[i];

        aiString materialName;
        assetMaterial->Get(AI_MATKEY_NAME, materialName);

        MaterialPtr material = Material::Create();
        material->name = std::format("{}.{}", sceneName, materialName.data);

        aiString alphaMode;
        if (aiReturn_SUCCESS == assetMaterial->Get("$mat.gltf.alphaMode", 0, 0, alphaMode))
        {
            if (strcmp(alphaMode.data, "OPAQUE") == 0)
            {
                material->props.alphaMode = AlphaMode::Opaque;
                material->props.alphaCutoff = 0.0f;
            }
            else if (strcmp(alphaMode.data, "BLEND") == 0)
            {
                material->props.alphaMode = AlphaMode::Blend;
                material->props.alphaCutoff = 0.0f;
            }
            else if (strcmp(alphaMode.data, "MASK") == 0)
            {
                material->props.alphaMode = AlphaMode::Mask;
                assetMaterial->Get("$mat.gltf.alphaCutoff", 0, 0, material->props.alphaCutoff);
            }
        }

        uint8_t isDoubleSided = 0;
        assetMaterial->Get(AI_MATKEY_TWOSIDED, isDoubleSided);
        material->props.isDoubleSided = isDoubleSided;

        assetMaterial->Get("$tex.scale", aiTextureType_NORMALS, 0, material->props.normalScale);

        aiColor3D albedo;
        aiReturn hr = assetMaterial->Get(AI_MATKEY_BASE_COLOR, albedo);
        material->props.albedo = glm::vec4(albedo.r, albedo.g, albedo.b, 1.0f);
        hr = assetMaterial->Get(AI_MATKEY_OPACITY, material->props.albedo.a);

        aiColor3D emissive;
        assetMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, emissive);
        material->props.emissiveValue = glm::vec4(emissive.r, emissive.g, emissive.b, 1.0f);
        assetMaterial->Get(AI_MATKEY_EMISSIVE_INTENSITY, material->props.emissiveValue.a);

        material->normalsTexture = LoadMaterialTexture(assetMaterial, aiTextureType_NORMALS, sceneFolder, cmdBuffer);
        material->emissiveTexture = LoadMaterialTexture(assetMaterial, aiTextureType_EMISSIVE, sceneFolder, cmdBuffer);
        material->occlusionTexture = LoadMaterialTexture(assetMaterial, aiTextureType_LIGHTMAP, sceneFolder, cmdBuffer);

        float glossiness = 1.0f;
        if (aiReturn_SUCCESS == assetMaterial->Get(AI_MATKEY_GLOSSINESS_FACTOR, glossiness))
        {
            material->props.workflow = Workflow::SpecularGlossiness;
            material->props.specular.a = glossiness;

            aiColor4D specular;
            if (aiReturn_SUCCESS == assetMaterial->Get(AI_MATKEY_COLOR_SPECULAR, specular))
            {
                material->props.specular.r = specular.r;
                material->props.specular.g = specular.g;
                material->props.specular.b = specular.b;
            }

            material->albedoTexture = LoadMaterialTexture(assetMaterial, aiTextureType_DIFFUSE, sceneFolder, cmdBuffer);
            material->specularTexture = LoadMaterialTexture(assetMaterial, aiTextureType_SPECULAR, sceneFolder, cmdBuffer);
        }
        else
        {
            material->props.workflow = Workflow::MetallicRoughness;

            material->props.aoMetRough = glm::vec4(1.0f);
            assetMaterial->Get(AI_MATKEY_METALLIC_FACTOR, material->props.aoMetRough.g);
            assetMaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, material->props.aoMetRough.b);

            material->albedoTexture = LoadMaterialTexture(assetMaterial, aiTextureType_BASE_COLOR, sceneFolder, cmdBuffer);
            material->metRoughTexture = LoadMaterialTexture(assetMaterial, aiTextureType_METALNESS, sceneFolder, cmdBuffer);
        }

        material->propsBuffer = Buffer::CreateStructured(sizeof(material->props), false);
        material->propsBuffer->SetName(material->name);

        material->props.albedoMap = material->albedoTexture ? material->albedoTexture->BindSRV() : 0;
        material->props.normalsMap = material->normalsTexture ? material->normalsTexture->BindSRV() : 0;
        material->props.aoRoughMetMap = material->metRoughTexture ? material->metRoughTexture->BindSRV() : 0;
        material->props.emissiveMap = material->emissiveTexture ? material->emissiveTexture->BindSRV() : 0;
        material->props.specularMap = material->specularTexture ? material->specularTexture->BindSRV() : 0;
        material->props.occlusionMap = material->occlusionTexture ? material->occlusionTexture->BindSRV() : 0;
        material->props.sampler = Sampler::GetLinearAnisotropy().GetBindSlot();

        cmdBuffer->CopyToBuffer(material->propsBuffer, &material->props, sizeof(material->props));

        materials.push_back(std::move(material));

        Log("Loaded material '{}'", materials.back()->name);
    }

    return materials;
}

std::vector<uint32_t> MeshLoader::RetrieveIndices(const aiMesh* assetMesh)
{
    uint32_t idxCount = assetMesh->mNumFaces;

    std::vector<uint32_t> indices;
    indices.reserve(idxCount);

    const aiFace* facesPtr = assetMesh->mFaces;
    for (uint32_t idx = 0; idx < idxCount; idx++)
    {
        indices.push_back(facesPtr[idx].mIndices[0]);
        indices.push_back(facesPtr[idx].mIndices[1]);
        indices.push_back(facesPtr[idx].mIndices[2]);
    }

    return indices;
}

std::vector<float> MeshLoader::RetrievePositions(const aiMesh* assetMesh)
{
    uint32_t vtxCount = assetMesh->mNumVertices;

    std::vector<float> vertices;
    vertices.reserve(vtxCount * 3);

    const aiVector3D* vtxPtr = assetMesh->mVertices;
    for (uint32_t vtxId = 0; vtxId < vtxCount; vtxId++)
    {
        aiVector3D vtx = vtxPtr[vtxId];
        vertices.push_back(vtx.x);
        vertices.push_back(vtx.y);
        vertices.push_back(vtx.z);
    }

    return vertices;
}

std::vector<int16_t> MeshLoader::RetrieveNormals(const aiMesh* assetMesh)
{
    uint32_t vtxCount = assetMesh->mNumVertices;

    std::vector<int16_t> normals;
    normals.reserve(vtxCount * 3);

    const aiVector3D* normalsPtr = assetMesh->mNormals;
    for (uint32_t vtx = 0; vtx < vtxCount; vtx++)
    {
        normals.push_back(glm::detail::toFloat16(normalsPtr[vtx].x));
        normals.push_back(glm::detail::toFloat16(normalsPtr[vtx].y));
        normals.push_back(glm::detail::toFloat16(normalsPtr[vtx].z));
    }

    return normals;
}

std::vector<int16_t> MeshLoader::RetrieveTangentsBitangents(const aiMesh* assetMesh)
{
    if (!assetMesh->mTangents || !assetMesh->mBitangents)
    {
        return {};
    }

    uint32_t vtxCount = assetMesh->mNumVertices;

    std::vector<int16_t> tangentsBitangents;
    tangentsBitangents.reserve(vtxCount * 6);

    const aiVector3D* tangentsPtr = assetMesh->mTangents;
    const aiVector3D* bitangentsPtr = assetMesh->mBitangents;
    for (uint32_t vtx = 0; vtx < vtxCount; vtx++)
    {
        tangentsBitangents.push_back(glm::detail::toFloat16(tangentsPtr[vtx].x));
        tangentsBitangents.push_back(glm::detail::toFloat16(tangentsPtr[vtx].y));
        tangentsBitangents.push_back(glm::detail::toFloat16(tangentsPtr[vtx].z));
        tangentsBitangents.push_back(glm::detail::toFloat16(bitangentsPtr[vtx].x));
        tangentsBitangents.push_back(glm::detail::toFloat16(bitangentsPtr[vtx].y));
        tangentsBitangents.push_back(glm::detail::toFloat16(bitangentsPtr[vtx].z));
    }

    return tangentsBitangents;
}

std::vector<int16_t> MeshLoader::RetrieveUV0(const aiMesh* assetMesh)
{
    if (!assetMesh->HasTextureCoords(0))
    {
        return {};
    }

    uint32_t vtxCount = assetMesh->mNumVertices;

    std::vector<int16_t> uvs;
    uvs.reserve(vtxCount * 2);

    const aiVector3D* uvsPtr = assetMesh->mTextureCoords[0];
    for (uint32_t vtx = 0; vtx < vtxCount; vtx++)
    {
        uvs.push_back(glm::detail::toFloat16(uvsPtr[vtx].x));
        uvs.push_back(glm::detail::toFloat16(uvsPtr[vtx].y));
    }

    return uvs;
}

std::vector<int16_t> MeshLoader::RetrieveColors(const aiMesh* assetMesh)
{
    if (!assetMesh->HasVertexColors(0))
    {
        return {};
    }

    uint32_t vtxCount = assetMesh->mNumVertices;

    std::vector<int16_t> colors;
    colors.reserve(vtxCount * 4);

    const aiColor4D* colorPtr = assetMesh->mColors[0];
    for (uint32_t colorId = 0; colorId < vtxCount; colorId++)
    {
        aiColor4D color = colorPtr[colorId];
        colors.push_back(glm::detail::toFloat16(color.r));
        colors.push_back(glm::detail::toFloat16(color.g));
        colors.push_back(glm::detail::toFloat16(color.b));
        colors.push_back(glm::detail::toFloat16(color.a));
    }

    return colors;
}

MeshLoader::MeshletData MeshLoader::BuildMeshlets(const std::vector<float>& positions, size_t vtxCount, const std::vector<uint32_t>& indices)
{
    const uint32_t vertexCountLimit = 64;
    const uint32_t triangleCountLimit = 84;

    MeshletData meshletData;

    size_t meshletsCount = meshopt_buildMeshletsBound(indices.size(), vertexCountLimit, triangleCountLimit);

    std::vector<meshopt_Meshlet> meshlets(meshletsCount);
    std::vector<uint32_t> meshletVertices(meshletsCount * vertexCountLimit);
    std::vector<uint8_t> meshletTriangles(meshletsCount * triangleCountLimit * 3);

    meshletsCount = meshopt_buildMeshlets(meshlets.data(), meshletVertices.data(), meshletTriangles.data(),
        indices.data(), indices.size(), positions.data(), vtxCount, 12, vertexCountLimit, triangleCountLimit, 1.0f);

    const meshopt_Meshlet& lastMeshlet = meshlets[meshletsCount - 1];

    meshletData.meshlets.reserve(meshletsCount);

    for (size_t i = 0; i < meshletsCount; i++)
    {
        Meshlet meshlet{};
        meshlet.vertexOffset = (uint32_t)meshletData.meshletVertices.size();
        meshlet.vertexCount = meshlets[i].vertex_count;
        meshlet.triangleOffset = (uint32_t)meshletData.meshletTriangles.size();
        meshlet.triangleCount = meshlets[i].triangle_count;

        uint32_t vertOffset = meshlets[i].vertex_offset;
        for (size_t j = 0; j < meshlet.vertexCount; j++)
        {
            meshletData.meshletVertices.push_back(meshletVertices[vertOffset + j]);
        }

        uint32_t triOffset = meshlets[i].triangle_offset;
        for (size_t j = 0; j < meshlet.triangleCount; j++)
        {
            uint32_t index =
                 meshletTriangles[triOffset + j * 3] |
                (meshletTriangles[triOffset + j * 3 + 1] << 8) |
                (meshletTriangles[triOffset + j * 3 + 2] << 16);

            meshletData.meshletTriangles.push_back(index);
        }

        meshopt_Bounds bounds = meshopt_computeMeshletBounds(&meshletVertices[meshlets[i].vertex_offset], &meshletTriangles[meshlets[i].triangle_offset],
            meshlet.triangleCount, positions.data(), positions.size(), 12);

        meshlet.center[0] = bounds.center[0];
        meshlet.center[1] = bounds.center[1];
        meshlet.center[2] = bounds.center[2];
        meshlet.radius = bounds.radius;

        meshlet.coneApex[0] = bounds.cone_apex[0];
        meshlet.coneApex[1] = bounds.cone_apex[1];
        meshlet.coneApex[2] = bounds.cone_apex[2];

        meshlet.coneAxis[0] = bounds.cone_axis[0];
        meshlet.coneAxis[1] = bounds.cone_axis[1];
        meshlet.coneAxis[2] = bounds.cone_axis[2];

        meshlet.cutoff = bounds.cone_cutoff;

        meshletData.meshlets.push_back(meshlet);
    }

    return meshletData;
}

std::vector<MeshPtr> MeshLoader::RetrieveMeshes(const aiScene* assetScene, std::string_view sceneName)
{
    ProfileFunction();

    std::vector<MeshPtr> meshes;

    CommandBufferPtr cmdBuffer = Renderer::Get()->GetLoadCmdBuffer();

    for (uint32_t i = 0; i < assetScene->mNumMeshes; i++)
    {
        aiMesh* assetMesh = assetScene->mMeshes[i];

        size_t vtxCount = assetMesh->mNumVertices;
        uint32_t idxCount = 3 * assetMesh->mNumFaces;

        VertexComponentFlags components = VertexComponentNone;

        std::vector<float> positions = RetrievePositions(assetMesh);
        std::vector<int16_t> normals = RetrieveNormals(assetMesh);
        std::vector<int16_t> tangentsBitangents = RetrieveTangentsBitangents(assetMesh);
        std::vector<int16_t> uvs = RetrieveUV0(assetMesh);
        std::vector<int16_t> colors = RetrieveColors(assetMesh);

        std::vector<uint32_t> indices(idxCount);
        std::vector<uint32_t> remap(idxCount);
        {
            std::vector<uint32_t> rawIndices = RetrieveIndices(assetMesh);
            meshopt_optimizeVertexCache(rawIndices.data(), rawIndices.data(), idxCount, vtxCount);

            vtxCount = meshopt_optimizeVertexFetchRemap(remap.data(), rawIndices.data(), idxCount, vtxCount);
            meshopt_remapIndexBuffer(indices.data(), rawIndices.data(), idxCount, remap.data());
        }

        meshopt_remapVertexBuffer(positions.data(), positions.data(), vtxCount, 3 * sizeof(positions[0]), remap.data());
        if (!normals.empty())
        {
            meshopt_remapVertexBuffer(normals.data(), normals.data(), vtxCount, 3 * sizeof(normals[0]), remap.data());
        }
        if (!tangentsBitangents.empty())
        {
            meshopt_remapVertexBuffer(tangentsBitangents.data(), tangentsBitangents.data(), vtxCount, 6 * sizeof(tangentsBitangents[0]), remap.data());
        }
        if (!uvs.empty())
        {
            meshopt_remapVertexBuffer(uvs.data(), uvs.data(), vtxCount, 2 * sizeof(uvs[0]), remap.data());
        }
        if (!colors.empty())
        {
            meshopt_remapVertexBuffer(colors.data(), colors.data(), vtxCount, 4 * sizeof(colors[0]), remap.data());
        }

        MeshletData meshletData = BuildMeshlets(positions, vtxCount, indices);

        int vertexStride = 3 * sizeof(positions[0]);
        vertexStride += 3 * sizeof(normals[0]);
        if (!tangentsBitangents.empty())
        {
            components |= VertexComponentTangentBitangents;
            vertexStride += 6 * sizeof(tangentsBitangents[0]);
        }
        if (!uvs.empty())
        {
            components |= VertexComponentUvs;
            vertexStride += 2 * sizeof(uvs[0]);
        }
        if (!colors.empty())
        {
            components |= VertexComponentColors;
            vertexStride += 4 * sizeof(colors[0]);
        }
        vertexStride = (vertexStride + 3) & ~3;

        std::vector<uint8_t> vertices;
        vertices.resize(vtxCount * vertexStride);

        for (int i = 0; i < vtxCount; i++)
        {
            uint8_t* dstPtr = vertices.data() + i * vertexStride;

            memcpy(dstPtr, &positions[i * 3], 3 * sizeof(positions[0]));
            dstPtr += 3 * sizeof(positions[0]);

            memcpy(dstPtr, &normals[i * 3], 3 * sizeof(normals[0]));
            dstPtr += 3 * sizeof(normals[0]);

            if (!tangentsBitangents.empty())
            {
                memcpy(dstPtr, &tangentsBitangents[i * 6], 6 * sizeof(tangentsBitangents[0]));
                dstPtr += 6 * sizeof(tangentsBitangents[0]);
            }
            if (!uvs.empty())
            {
                memcpy(dstPtr, &uvs[i * 2], 2 * sizeof(uvs[0]));
                dstPtr += 2 * sizeof(uvs[0]);
            }
            if (!colors.empty())
            {
                memcpy(dstPtr, &colors[i * 4], 4 * sizeof(colors[0]));
                dstPtr += 4 * sizeof(colors[0]);
            }
        }

        std::string meshName = std::format("{}.{}", sceneName, assetMesh->mName.data);

        MeshPtr mesh = Mesh::Create();
        mesh->components = components;
        mesh->meshletsCount = (int)meshletData.meshlets.size();
        mesh->indexBuffer = Buffer::CreateStructured(sizeof(uint32_t) * idxCount, false);
        mesh->meshlets = Buffer::CreateStructured(sizeof(Meshlet) * meshletData.meshlets.size(), false);
        mesh->meshletVertices = Buffer::CreateStructured(meshletData.meshletVertices.size() * sizeof(meshletData.meshletVertices[0]), false);
        mesh->meshletTriangles = Buffer::CreateStructured(meshletData.meshletTriangles.size() * sizeof(meshletData.meshletTriangles[0]), false);
        mesh->positions = Buffer::CreateStructured(vtxCount * sizeof(positions[0]) * 3, false);
        mesh->vertices = Buffer::CreateStructured(vtxCount * vertexStride, false);

        mesh->indexBuffer->SetName("VtxIndices: " + meshName);
        mesh->meshlets->SetName("Meshlets: " + meshName);
        mesh->meshletVertices->SetName("MeshletVertices: " + meshName);
        mesh->meshletTriangles->SetName("MeshletTriangles: " + meshName);
        mesh->positions->SetName("VtxPos: " + meshName);
        mesh->vertices->SetName("Vertices: " + meshName);

        cmdBuffer->CopyToBuffer(mesh->indexBuffer, indices.data(), mesh->indexBuffer->GetSize());
        cmdBuffer->CopyToBuffer(mesh->meshlets, meshletData.meshlets.data(), mesh->meshlets->GetSize());
        cmdBuffer->CopyToBuffer(mesh->meshletVertices, meshletData.meshletVertices.data(), mesh->meshletVertices->GetSize());
        cmdBuffer->CopyToBuffer(mesh->meshletTriangles, meshletData.meshletTriangles.data(), mesh->meshletTriangles->GetSize());
        cmdBuffer->CopyToBuffer(mesh->positions, positions.data(), mesh->positions->GetSize());
        cmdBuffer->CopyToBuffer(mesh->vertices, vertices.data(), mesh->vertices->GetSize());

        meshes.push_back(mesh);
    }

    return meshes;
}

void MeshLoader::PopulateNode(const aiScene* assetScene, const aiNode* assetNode, std::string_view sceneName, Entity node, const glm::mat4& parentTransform, std::vector<MaterialPtr>& materials, std::vector<MeshPtr>& meshes)
{
    node.GetComponent<TagComponent>().tag = assetNode->mName.data;
    TransformComponent& transformComponent = node.GetComponent<TransformComponent>();
    aiVector3D translation, scale, rotation;
    assetNode->mTransformation.Decompose(scale, rotation, translation);

    transformComponent.UpdateTranslation(glm::vec3(translation[0], translation[1], translation[2]));
    transformComponent.UpdateScale(glm::vec3(scale[0], scale[1], scale[2]));
    transformComponent.UpdateRotation(glm::degrees(glm::vec3(rotation[0], rotation[1], rotation[2])));

    glm::mat4 globalTransform = parentTransform * transformComponent.transform;;

    if (assetNode->mNumMeshes)
    {
        MeshComponent& meshComponent = node.GetOrAddComponent<MeshComponent>();
        meshComponent.transforms.localTransform = transformComponent.transform;
        meshComponent.transforms.globalTransform = globalTransform;
        meshComponent.transforms.transposeInverseGlobalTransform = glm::transpose(glm::inverse(globalTransform));
        meshComponent.transformsBuffer = Buffer::CreateStructured(sizeof(MeshComponent::Transforms), false);
        meshComponent.transformsBuffer->SetName(std::format("Node matrices: {}.{}", sceneName, assetNode->mName.data));

        Renderer::Get()->GetLoadCmdBuffer()->CopyToBuffer(meshComponent.transformsBuffer, &meshComponent.transforms, sizeof(MeshComponent::Transforms));

        for (int i = 0; i < (int)assetNode->mNumMeshes; i++)
        {
            int meshIndex = assetNode->mMeshes[i];
            int materialIndex = assetScene->mMeshes[meshIndex]->mMaterialIndex;

            MaterialPtr& material = materials[materialIndex];

            RenderObjectPtr renderObject = RenderObject::Create();
            renderObject->perInstanceBuffer = node.GetComponent<MeshComponent>().transformsBuffer;
            renderObject->mesh = meshes[meshIndex];
            renderObject->material = material;
            meshComponent.renderObjects.push_back(renderObject);
        }
    }

    for (uint32_t i = 0; i < assetNode->mNumChildren; i++)
    {
        Entity child = Application::Get()->GetLevel()->CreateEntity("", node);

        PopulateNode(assetScene, assetNode->mChildren[i], sceneName, child, globalTransform, materials, meshes);
    }
}

Entity MeshLoader::RetrieveNodes(const aiScene* assetScene, std::vector<MaterialPtr>& materials, std::vector<MeshPtr>& meshes, std::string_view sceneName)
{
    ProfileFunction();

    glm::mat4 scale = glm::mat4(1.0f);

    Entity root = Application::Get()->GetLevel()->CreateEntity(sceneName);
    TransformComponent& transformComponent = root.GetComponent<TransformComponent>();
    transformComponent.UpdateRotation(glm::vec3(90.0f, 0.0f, 0.0f));

    if (assetScene->mRootNode)
    {
        Entity secondaryRoot = Application::Get()->GetLevel()->CreateEntity("", root);

        PopulateNode(assetScene, assetScene->mRootNode, sceneName, secondaryRoot, transformComponent.transform, materials, meshes);
    }

    return root;
}