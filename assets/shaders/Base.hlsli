#ifndef BASE_H
#define BASE_H

// DECLARATION
#define DEFINE_TEXTURE_TYPES_AND_FORMATS_SLOTS(textureType, reg)     \
    textureType<int>    g##_##textureType##int[]    : register(reg); \
    textureType<uint>   g##_##textureType##uint[]   : register(reg); \
    textureType<float>  g##_##textureType##float[]  : register(reg); \
    textureType<int2>   g##_##textureType##int2[]   : register(reg); \
    textureType<uint2>  g##_##textureType##uint2[]  : register(reg); \
    textureType<float2> g##_##textureType##float2[] : register(reg); \
    textureType<int3>   g##_##textureType##int3[]   : register(reg); \
    textureType<uint3>  g##_##textureType##uint3[]  : register(reg); \
    textureType<float3> g##_##textureType##float3[] : register(reg); \
    textureType<int4>   g##_##textureType##int4[]   : register(reg); \
    textureType<uint4>  g##_##textureType##uint4[]  : register(reg); \
    textureType<float4> g##_##textureType##float4[] : register(reg);                 


DEFINE_TEXTURE_TYPES_AND_FORMATS_SLOTS(Texture1D,   t0)
DEFINE_TEXTURE_TYPES_AND_FORMATS_SLOTS(Texture2D,   t0)
DEFINE_TEXTURE_TYPES_AND_FORMATS_SLOTS(Texture3D,   t0)
DEFINE_TEXTURE_TYPES_AND_FORMATS_SLOTS(TextureCube, t0)
DEFINE_TEXTURE_TYPES_AND_FORMATS_SLOTS(RWTexture1D, u1048576)
DEFINE_TEXTURE_TYPES_AND_FORMATS_SLOTS(RWTexture2D, u1048576)
DEFINE_TEXTURE_TYPES_AND_FORMATS_SLOTS(RWTexture3D, u1048576)

ByteAddressBuffer   g_ByteAddressBuffer[]   : register(t2097152);
RWByteAddressBuffer g_RWByteAddressBuffer[] : register(u2097152);

SamplerState g_SamplerState[] : register(s3145728);


// DESCRIPTOR HEAP EMULATION
struct ByteBufferHandle { uint internalIndex; };
struct RWByteBufferHandle { uint internalIndex; };
template<typename T> struct Texture1DHandle { uint internalIndex; };
template<typename T> struct Texture2DHandle { uint internalIndex; };
template<typename T> struct Texture3DHandle { uint internalIndex; };
template<typename T> struct TextureCubeHandle { uint internalIndex; };
template<typename T> struct RWTexture1DHandle { uint internalIndex; };
template<typename T> struct RWTexture2DHandle { uint internalIndex; };
template<typename T> struct RWTexture3DHandle { uint internalIndex; };
struct SamplerStateHandle { uint internalIndex; };

#define TEXTURE_TYPE_TEMPLATE_SPECIALIZATION_DECL(resourceType, registerName, valueType, handleName)          \
    resourceType<valueType> operator[](handleName<valueType> identifier)                                      \
    {                                                                                                         \
        return registerName##valueType[NonUniformResourceIndex(identifier.internalIndex)];                    \
    }


#define TEXTURE_TYPE_TEMPLATE_SPECIALIZATION_DECL_MULTI(resourceType)                                         \
    TEXTURE_TYPE_TEMPLATE_SPECIALIZATION_DECL(resourceType, g##_##resourceType, int,    resourceType##Handle) \
    TEXTURE_TYPE_TEMPLATE_SPECIALIZATION_DECL(resourceType, g##_##resourceType, uint,   resourceType##Handle) \
    TEXTURE_TYPE_TEMPLATE_SPECIALIZATION_DECL(resourceType, g##_##resourceType, float,  resourceType##Handle) \
    TEXTURE_TYPE_TEMPLATE_SPECIALIZATION_DECL(resourceType, g##_##resourceType, int2,   resourceType##Handle) \
    TEXTURE_TYPE_TEMPLATE_SPECIALIZATION_DECL(resourceType, g##_##resourceType, uint2,  resourceType##Handle) \
    TEXTURE_TYPE_TEMPLATE_SPECIALIZATION_DECL(resourceType, g##_##resourceType, float2, resourceType##Handle) \
    TEXTURE_TYPE_TEMPLATE_SPECIALIZATION_DECL(resourceType, g##_##resourceType, int3,   resourceType##Handle) \
    TEXTURE_TYPE_TEMPLATE_SPECIALIZATION_DECL(resourceType, g##_##resourceType, uint3,  resourceType##Handle) \
    TEXTURE_TYPE_TEMPLATE_SPECIALIZATION_DECL(resourceType, g##_##resourceType, float3, resourceType##Handle) \
    TEXTURE_TYPE_TEMPLATE_SPECIALIZATION_DECL(resourceType, g##_##resourceType, int4,   resourceType##Handle) \
    TEXTURE_TYPE_TEMPLATE_SPECIALIZATION_DECL(resourceType, g##_##resourceType, uint4,  resourceType##Handle) \
    TEXTURE_TYPE_TEMPLATE_SPECIALIZATION_DECL(resourceType, g##_##resourceType, float4, resourceType##Handle)


struct VulkanResourceDescriptorHeapInternal
{
    ByteAddressBuffer operator[](ByteBufferHandle identifier)
    {
        return g_ByteAddressBuffer[NonUniformResourceIndex(identifier.internalIndex)];
    }

    RWByteAddressBuffer operator[](RWByteBufferHandle identifier)
    {
        return g_RWByteAddressBuffer[NonUniformResourceIndex(identifier.internalIndex)];
    }

    SamplerState operator[](SamplerStateHandle identifier)
    {
        return g_SamplerState[NonUniformResourceIndex(identifier.internalIndex)];
    }

    TEXTURE_TYPE_TEMPLATE_SPECIALIZATION_DECL_MULTI(Texture1D);
    TEXTURE_TYPE_TEMPLATE_SPECIALIZATION_DECL_MULTI(Texture2D);
    TEXTURE_TYPE_TEMPLATE_SPECIALIZATION_DECL_MULTI(Texture3D);
    TEXTURE_TYPE_TEMPLATE_SPECIALIZATION_DECL_MULTI(TextureCube);
    TEXTURE_TYPE_TEMPLATE_SPECIALIZATION_DECL_MULTI(RWTexture1D);
    TEXTURE_TYPE_TEMPLATE_SPECIALIZATION_DECL_MULTI(RWTexture2D);
    TEXTURE_TYPE_TEMPLATE_SPECIALIZATION_DECL_MULTI(RWTexture3D);
};

static VulkanResourceDescriptorHeapInternal VkResourceDescriptorHeap;


#define DESCRIPTOR_HEAP(handleType, handle) VkResourceDescriptorHeap[(handleType)handle]


// DESCRIPTOR HANDLE
#define VALIDATE_HANDLE() \
    //if (handle.Read() == 0) { return; }


#define VALIDATE_HANDLE_RET(type) \
    //if (handle.Read() == 0) { return (type)0; }


struct RenderResourceHandle
{
    uint index;

    uint Read() { return index; }

    bool IsValid() { return index != 0; }
};


// DESCRIPTORS
struct ArrayBuffer
{
    RenderResourceHandle handle;

    bool IsValid()
    {
        return handle.IsValid();
    }

    template<typename ReadStructure>
    ReadStructure Load(uint index)
    {
        VALIDATE_HANDLE_RET(ReadStructure);
        ByteAddressBuffer buffer = DESCRIPTOR_HEAP(ByteBufferHandle, handle.Read());
        ReadStructure result = buffer.Load<ReadStructure>(sizeof(ReadStructure) * index);
        return result;
    }

    template<typename ReadStructure>
    ReadStructure Load()
    {
        VALIDATE_HANDLE_RET(ReadStructure);
        ByteAddressBuffer buffer = DESCRIPTOR_HEAP(ByteBufferHandle, handle.Read());
        ReadStructure result = buffer.Load<ReadStructure>(0);
        return result;
    }
};

struct RWArrayBuffer : ArrayBuffer
{
    template<typename WriteStructure>
    void Store(uint index, WriteStructure data)
    {
        VALIDATE_HANDLE();
        RWByteAddressBuffer buffer = DESCRIPTOR_HEAP(RWByteBufferHandle, handle.Read());
        WriteStructure result = buffer.Store<WriteStructure>(sizeof(WriteStructure) * index, data);
    }
};

struct Sampler
{
    RenderResourceHandle handle;

    SamplerState Get()
    {
        return DESCRIPTOR_HEAP(SamplerStateHandle, handle.Read());
    }
};

struct Texture
{
    RenderResourceHandle handle;

    bool IsValid()
    {
        return handle.IsValid();
    }

    template<typename TextureValue>
    TextureValue Load2D(uint2 pos)
    {
        VALIDATE_HANDLE_RET(TextureValue);
        Texture2D<TextureValue> texture = DESCRIPTOR_HEAP(Texture2DHandle<TextureValue>, handle.Read());
        return texture.Load(uint3(pos, 0));
    }
 
    template<typename TextureValue>
    TextureValue Load3D(uint3 pos)
    {
        VALIDATE_HANDLE_RET(TextureValue);
        Texture3D<TextureValue> texture = DESCRIPTOR_HEAP(Texture3DHandle<TextureValue>, handle.Read());
        return texture.Load(uint4(pos, 0));
    }
 
    template<typename TextureValue>
    TextureValue Sample2D(SamplerState s, float2 uv)
    {
        VALIDATE_HANDLE_RET(TextureValue);
        Texture2D<TextureValue> texture = DESCRIPTOR_HEAP(Texture2DHandle<TextureValue>, handle.Read());
        return texture.Sample(s, uv);
    }
 
    template<typename TextureValue>
    TextureValue Sample3D(SamplerState s, float3 uv)
    {
        VALIDATE_HANDLE_RET(TextureValue);
        Texture3D<TextureValue> texture = DESCRIPTOR_HEAP(Texture3DHandle<TextureValue>, handle.Read());
        return texture.Sample(s, uv);
    }

    template<typename TextureValue>
    TextureValue SampleCube(SamplerState s, float3 uv)
    {
        VALIDATE_HANDLE_RET(TextureValue);
        TextureCube<TextureValue> texture = DESCRIPTOR_HEAP(TextureCubeHandle<TextureValue>, handle.Read());
        return texture.Sample(s, uv);
    }

    template<typename TextureValue>
    TextureValue Sample2DLevel(SamplerState s, float2 uv, float level)
    {
        VALIDATE_HANDLE_RET(TextureValue);
        Texture2D<TextureValue> texture = DESCRIPTOR_HEAP(Texture2DHandle<TextureValue>, handle.Read());
        return texture.SampleLevel(s, uv, level);
    }

    template<typename TextureValue>
    TextureValue SampleCubeLevel(SamplerState s, float3 uv, float level)
    {
        VALIDATE_HANDLE_RET(TextureValue);
        TextureCube<TextureValue> texture = DESCRIPTOR_HEAP(TextureCubeHandle<TextureValue>, handle.Read());
        return texture.SampleLevel(s, uv, level);
    }
};

struct RWTexture
{
    RenderResourceHandle handle;

    template<typename TextureValue>
    void Store2D(uint2 uv, TextureValue value)
    {
        VALIDATE_HANDLE();
        RWTexture2D<TextureValue> texture = DESCRIPTOR_HEAP(RWTexture2DHandle<TextureValue>, handle.Read());
        texture[uv] = value;
    }

    template<typename TextureValue>
    TextureValue Load2D(int2 pos)
    {
        VALIDATE_HANDLE_RET(TextureValue);
        RWTexture2D<TextureValue> texture = DESCRIPTOR_HEAP(RWTexture2DHandle<TextureValue>, handle.Read());
        return texture.Load(pos);
    }
};


// PUSH CONSTANTS
#define PUSH_CONSTANTS(pushVarType, pushVar) \
[[vk::push_constant]] pushVarType pushVar;

#endif // BASE_H