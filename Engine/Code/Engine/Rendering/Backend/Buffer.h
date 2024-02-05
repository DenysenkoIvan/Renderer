#pragma once

#include <memory>

#include <Framework/Common.h>

#include "VulkanImpl/VulkanBuffer.h"

#include "Common.h"
#include "ShaderUtils.h"

enum BufferUsageFlagBits : uint32_t
{
    BufferUsageNone         = 0,
    BufferUsageTransferSrc  = 1,
    BufferUsageTransferDst  = 2,
    BufferUsageStorageRead  = 8,
    BufferUsageStorageWrite = 16
};
using BufferUsageFlags = uint32_t;

struct BufferState
{
    BufferUsageFlags usage = BufferUsageNone;
    ShaderStageFlags stages = ShaderStageNone;

    BufferState(BufferUsageFlags usage = BufferUsageNone, ShaderStageFlags stages = ShaderStageNone)
        : usage(usage), stages(stages) {}

    bool IsUndefined() const
    {
        return usage == 0 && stages == 0;
    }
};

class Buffer;
using BufferPtr = std::shared_ptr<Buffer>;

class Buffer
{
public:
    NON_COPYABLE_MOVABLE(Buffer);

    Buffer(BufferUsageFlags usage, int64_t size, bool onGpu);
    ~Buffer();

    void SetName(std::string_view name);
    const std::string& GetName() const;

    int64_t GetSize() const;

    int BindSRV();
    int BindUAV();

    void* Map();
    void Unmap();

    const BufferState& GetState() const { return m_state; }
    void SetState(const BufferState& state) { m_state = state; }

    VulkanBuffer& GetBuffer();

    static BufferPtr CreateStaging(int64_t size);
    static BufferPtr CreateStructured(int64_t size, bool isWritable);

private:
    void BindDescriptor();

private:
    BufferUsageFlags m_usage = BufferUsageNone;
    VulkanBuffer m_buffer;
    BufferState m_state;

    int m_descriptorSlot = -1;

#if !defined(RELEASE_BUILD)
    std::string m_name;
#endif
};