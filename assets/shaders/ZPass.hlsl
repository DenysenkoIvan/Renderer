#include "Common.hlsli"
#include "ZPassCommon.hlsli"

#if defined(USE_MESH_SHADING)
    //#define MESH_DEBUG
#endif

struct VertToPix
{
    float4 Position : SV_Position;
    float3 WorldPosition : WORLD_POS;
    float3 Normal : NORMAL;

    #if defined(USE_UV)
        float2 TC : TEXCOORD;
    #endif

    #if defined(USE_TANGENTS_BITANGENTS)
        float3x3 TBN : TBNMat;
    #endif

    #if defined(USE_VERTEX_COLOR)
        float4 Color : COLOR;
    #endif

    #if defined(MESH_DEBUG)
        float3 MeshletColor : MESHLET_COLOR;
    #endif
};

struct PixOut
{
    float4 Color : SV_Target0;
};

#if defined(USE_MESH_SHADING)

uint hash(uint a)
{
   a = (a + 0x7ed55d16) + (a << 12);
   a = (a ^ 0xc761c23c) ^ (a >> 19);
   a = (a + 0x165667b1) + (a << 5);
   a = (a + 0xd3a2646c) ^ (a << 9);
   a = (a + 0xfd7046c5) + (a << 3);
   a = (a ^ 0xb55a4f09) ^ (a >> 16);
   return a;
}

[numthreads(THREADS_PER_GROUP, 1, 1)]
[OutputTopology("triangle")]
void MainMS(in uint3 groupId : SV_GroupID,
            in uint3 groupThreadId : SV_GroupThreadID,
            in payload Payload pl,
            out vertices VertToPix vertices[MAX_VERTICES_COUNT],
            out indices uint3 triangles[MAX_TRIANGLES_COUNT])
{
    const uint meshletId = pl.meshletIndices[groupId.x];

    if (meshletId >= drawData.meshletCount)
    {
        return;
    }

    Meshlet meshlet = drawData.meshlets.Load<Meshlet>(meshletId);

    const uint vtxCount = meshlet.vertexCount;
    const uint triangleCount = meshlet.triangleCount;

    if (groupThreadId.x == 0)
    {
        SetMeshOutputCounts(vtxCount, triangleCount);
    }

    const ModelMatrix model = drawData.perInstanceBuffer.Load<ModelMatrix>();
    const float4x4 projView = drawData.perFrameBuffer.Load<PerFrameData>().projView;

    const uint vertexOffset = meshlet.vertexOffset;
    for (uint i = groupThreadId.x; i < vtxCount; i += THREADS_PER_GROUP)
    {
        const uint vertexIndex = drawData.meshletVertices.Load<uint>(vertexOffset + i);

        VertToPix OUT = (VertToPix)0;

        Vertex vertex = drawData.vertices.Load<Vertex>(vertexIndex);

        float3 localPos = vertex.Position;
        float4 worldPosition = mul(model.globalTransform, float4(localPos, 1.0f));
        OUT.Position = mul(projView, worldPosition);
        OUT.Position.y *= -1.0f;
        OUT.WorldPosition = worldPosition.xyz / worldPosition.w;

        half3 localNormal = half3(vertex.NormalX, vertex.NormalY, vertex.NormalZ);
        half4 worldNormal = half4(mul(model.globalTransform, half4(localNormal, 0.0f)));
        OUT.Normal = normalize(worldNormal.xyz);

        #if defined(USE_TANGENTS_BITANGENTS)
            half3 tangent = half3(vertex.TangentX, vertex.TangentY, vertex.TangentZ);
            half3 bitangent = half3(vertex.BitangentX, vertex.BitangentY, vertex.BitangentZ);

            half3 T = normalize(half3(mul(model.transpInvGlobalTransform, half4(tangent, 0.0f)).xyz));
            half3 B = normalize(half3(mul(model.transpInvGlobalTransform, half4(bitangent, 0.0f)).xyz));
            half3 N = normalize(half3(mul(model.transpInvGlobalTransform, half4(localNormal, 0.0f)).xyz));

            OUT.TBN = transpose(float3x3(T, B, N));
        #endif

        #if defined(USE_UV)
            OUT.TC = float2(vertex.TC_U, vertex.TC_V);
        #endif

        #if defined(USE_VERTEX_COLOR)
            OUT.Color = float4(vertex.ColorR, vertex.ColorG, vertex.ColorB, vertex.ColorA);
        #endif

        #if defined(MESH_DEBUG)
            uint mhash = hash(meshletId);
            OUT.MeshletColor = float3(float(mhash & 255), float((mhash >> 8) & 255), float((mhash >> 16) & 255)) / 255.0f;
        #endif

        vertices[i] = OUT;
    }

    const uint triangleOffset = meshlet.triangleOffset;
    for (uint i = groupThreadId.x; i < triangleCount; i += THREADS_PER_GROUP)
    {
        uint indices = drawData.meshletIndices.Load<uint>(triangleOffset + i);
        triangles[i] = uint3(indices & 0xff, (indices >> 8) & 0xff, (indices >> 16) & 0xff);
    }
}

#else // USE_MESH_SHADING

VertToPix MainVS(in uint vertexId : SV_VertexID)
{
    VertToPix OUT = (VertToPix)0;

    uint vertexIndex = vertexId;
    if (drawData.indices.IsValid())
    {
        vertexIndex = drawData.indices.Load<uint>(vertexId);
    }

    Vertex vertex = drawData.vertices.Load<Vertex>(vertexIndex);
    ModelMatrix model = drawData.perInstanceBuffer.Load<ModelMatrix>();

    float3 localPos = vertex.Position;
    float4 worldPosition = mul(model.globalTransform, float4(localPos, 1.0f));
    OUT.Position = mul(drawData.perFrameBuffer.Load<PerFrameData>().projView, worldPosition);
    OUT.WorldPosition = worldPosition.xyz / worldPosition.w;

    half3 localNormal = half3(vertex.NormalX, vertex.NormalY, vertex.NormalZ);
    half4 worldNormal = half4(mul(model.globalTransform, half4(localNormal, 0.0f)));
    OUT.Normal = normalize(worldNormal.xyz);

    #if defined(USE_TANGENTS_BITANGENTS)
        half3 tangent = half3(vertex.TangentX, vertex.TangentY, vertex.TangentZ);
        half3 bitangent = half3(vertex.BitangentX, vertex.BitangentY, vertex.BitangentZ);

        half3 T = normalize(half3(mul(model.transpInvGlobalTransform, half4(tangent, 0.0f)).xyz));
        half3 B = normalize(half3(mul(model.transpInvGlobalTransform, half4(bitangent, 0.0f)).xyz));
        half3 N = normalize(half3(mul(model.transpInvGlobalTransform, half4(localNormal, 0.0f)).xyz));

        OUT.TBN = transpose(float3x3(T, B, N));
    #endif

    #if defined(USE_UV)
        OUT.TC = float2(vertex.TC_U, vertex.TC_V);
    #endif

    #if defined(USE_VERTEX_COLOR)
        OUT.Color = float4(vertex.ColorR, vertex.ColorG, vertex.ColorB, vertex.ColorA);
    #endif

    return OUT;
}

#endif // USE_MESH_SHADING

float4 GetAldebo(in VertToPix IN, in MaterialProps materialProps)
{
    float4 albedo = materialProps.albedo;

    #if defined(USE_UV)
        if (materialProps.albedoMap.IsValid())
        {
            albedo *= materialProps.albedoMap.Sample2D<float4>(materialProps.samplerDesc.Get(), IN.TC);
        }

        #ifdef USE_VERTEX_COLOR
            albedo *= IN.Color;
        #endif
    #endif
    
    return albedo;
}

void GetAoRoughMet(in VertToPix IN, in MaterialProps materialProps, out float rough, out float met)
{
    if (materialProps.workflow == (int)Workflow::MetallicRoughness)
    {
        rough = materialProps.aoMetRough.g;
        met = materialProps.aoMetRough.b;

        #if defined(USE_UV)
            if (materialProps.aoRoughMetMap.IsValid())
            {
                float4 val = materialProps.aoRoughMetMap.Sample2D<float4>(materialProps.samplerDesc.Get(), IN.TC);
                rough *= val.g;
                met *= val.b;
            }
        #endif
    }
    else
    {
        met = 0.0f;
        #if defined(USE_UV)
            float gloss = 1.0f;
            if (materialProps.specularMap.IsValid())
            {
                gloss = materialProps.specularMap.Sample2D<float4>(materialProps.samplerDesc.Get(), IN.TC).a;

            }
            rough = 1.0f - gloss * materialProps.specular.a;
        #else
            rough = 1.0f - materialProps.specular.a;
        #endif
    }
}

float3 GetNormal(in VertToPix IN, in MaterialProps materialProps)
{
    float3 normal = 0.0f;

    #if defined(USE_UV)
        if (materialProps.normalsMap.IsValid())
        {
            normal = materialProps.normalsMap.Sample2D<float4>(materialProps.samplerDesc.Get(), IN.TC).xyz * 2.0f - 1.0f;

            normal.x *= materialProps.normalScale;
            normal.y *= materialProps.normalScale;

            #if defined(USE_TANGENTS_BITANGENTS)
                normal = mul(IN.TBN, normal);
            #endif
        }
        else
    #endif
        {
            normal = IN.Normal;
        }

    normal = normalize(normal);

    return normal;
}

float3 GetSpecularColor(in VertToPix IN, in MaterialProps materialProps)
{
    if (materialProps.workflow == (int)Workflow::MetallicRoughness)
    {
        return materialProps.aoMetRough.b;
    }

    float3 specularFactor = materialProps.specular.rgb;
    #if defined(USE_UV)
        if (materialProps.specularMap.IsValid())
        {
            specularFactor *= materialProps.specularMap.Sample2D<float4>(materialProps.samplerDesc.Get(), IN.TC).rgb;
        }
    #endif
    
    return specularFactor;
}

float GetOcclusion(in VertToPix IN, in MaterialProps materialProps)
{
    #if defined(USE_UV)
        if (materialProps.occlusionMap.IsValid())
        {
            return materialProps.occlusionMap.Sample2D<float4>(materialProps.samplerDesc.Get(), IN.TC).r;
        }
    #endif
    
    return 1.0f;
}

float3 GetEmissive(float2 TC, in MaterialProps materialProps)
{
    #if defined(USE_UV)
        float4 emissive = materialProps.emissiveMap.Sample2D<float4>(materialProps.samplerDesc.Get(), TC);
        return float3(emissive.rgb * emissive.a);
    #else
        return materialProps.emissive.rgb * materialProps.emissive.a;
    #endif
}

float geometrySmith(float NdotV, float NdotL, float roughness) {
    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f;
    float ggx1 = NdotV / (NdotV * (1.0f - k) + k);
    float ggx2 = NdotL / (NdotL * (1.0f - k) + k);
    return ggx1 * ggx2;
}

float3 fresnelSchlick(float HdotV, float3 F0) {
    return F0 + (1.0f - F0) * pow(saturate(1.0f - HdotV), 5.0f);
}

float3 CalculateDiffuseIBLFraction(float NdotV, float3 F0, float3 albedo)
{
    float3 F = fresnelSchlick(NdotV, F0);
    float specularColorMax = max(max(F0.r, F0.g), F0.b);
    float3 diffuseColor = (1.0f - specularColorMax) * albedo;
    return (1.0f - max(max(F.r, F.g), F.b)) * diffuseColor;
}

float3 FresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    return F0 + (max(1.0f - roughness, F0) - F0) * pow(clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0f);
}

PixOut MainPS(VertToPix IN)
{
    PixOut OUT = (PixOut)0;

    #if defined(MESH_DEBUG)
        OUT.Color = float4(IN.MeshletColor, 1.0f);
        return OUT;
    #endif

    float2 TC = 0.5f;
    #if defined(USE_UV)
        TC = IN.TC;
    #endif

    PerFrameData perFrame = drawData.perFrameBuffer.Load<PerFrameData>();
    MaterialProps materialProps = drawData.materialProps.Load<MaterialProps>();

    float4 albedo = GetAldebo(IN, materialProps);
    if (materialProps.alphaMode == (int)AlphaMode::Mask && albedo.a < materialProps.alphaCutoff)
    {
        discard;
    }

    float roughness = 0.5f;
    float metallness = 0.5f;
    GetAoRoughMet(IN, materialProps, roughness, metallness);
    float ao = GetOcclusion(IN, materialProps);

    float3 F0 = 0.04f;
    if (materialProps.workflow == (int)Workflow::MetallicRoughness)
    {
        F0 = lerp(F0, albedo.rgb, metallness);
    }
    else
    {
        F0 = GetSpecularColor(IN, materialProps);
    }

    float3 radiance = perFrame.lightColorIntensity.rgb * perFrame.lightColorIntensity.a;

    float3 N = GetNormal(IN, materialProps);
    float3 V = normalize(IN.WorldPosition - perFrame.cameraPosition.xyz);
    float3 L = perFrame.lightDirection.xyz;
    float3 H = normalize(-V - L);

    float NdotV = max(dot(N, -V), 0.0000001f);
    float NdotL = max(dot(N, -L), 0.0000001f);
    float HdotV = max(dot(H, -V), 0.0f);
    float NdotH = max(dot(N, H), 0.0f);

    float3 F = fresnelSchlick(HdotV, F0);
    float D = DistributionGGX(NdotH, roughness);
    float G = geometrySmith(NdotV, NdotL, roughness);

    float specularColorMax = max(max(F0.r, F0.g), F0.b);
    float3 diffuseColor = (1.0f - specularColorMax) * albedo.rgb;
    float3 diffuse = (1.0f - max(max(F.r, F.g), F.b)) * diffuseColor / PI;
    float3 specular = F * G * D / (4.0f * NdotL * NdotV);

    float3 Lo = (diffuse + specular) * radiance * NdotL;
    
    float3 emissive = GetEmissive(TC, materialProps);

    float3 ambient = 0.0f;
    if (perFrame.convolutedCubemapTexture.IsValid())
    {
        float3 F_ibl = FresnelSchlickRoughness(max(NdotV, 0.0f), F0, roughness);

        float3 kS = F_ibl;
        float3 kD = 1.0f - kS;
        kD *= 1.0f - metallness;

        float3 irradiance = SampleCubemap(perFrame.convolutedCubemapTexture, perFrame.samplerDesc, N).rgb;
        float3 diffuse_ibl = irradiance * albedo.rgb;

        const float MAX_REFLECTION_LOD = 4.0f;
        float3 prefilteredColor = SamplePrefilteredCubemapLinear(perFrame.prefilteredCubemapTextures, perFrame.samplerDesc, reflect(V, N), roughness * MAX_REFLECTION_LOD).rgb;
        float2 envBRDF = perFrame.brdfLutTexture.Sample2D<float2>(perFrame.samplerDesc.Get(), float2(max(NdotV, 0.0f), roughness));
        float3 specular_ibl = prefilteredColor * (F_ibl * envBRDF.x + envBRDF.y);

        ambient += kD * diffuse_ibl + specular_ibl;
        // Debug utils
        //ambient += specular_ibl;
        //ambient += kD * diffuse_ibl;
    }
    ambient *= ao;

    OUT.Color = float4(Lo + ambient + emissive, albedo.a);
    // Debug utils
    //OUT.Color = float4(N * 0.5f + 0.5f, 1.0f);
    //OUT.Color = float4(albedo.rgb, 1.0f);
    //OUT.Color = float4(roughness, roughness, roughness, 1.0f);
    //OUT.Color = float4(F0, 1.0f);

    return OUT;
}