#pragma once

#include <cstdint>

inline uint64_t JenkinsHashBegin(const void* data, size_t len)
{
    uint64_t hash = 0;

    const uint8_t* key = (uint8_t*)data;
    for (size_t i = 0; i < len; i++)
    {
        hash += key[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }

    return hash;
}

inline uint64_t JenkinsHashContinue(uint64_t hash, const void* data, size_t len)
{
    const uint8_t* key = (uint8_t*)data;
    for (size_t i = 0; i < len; i++)
    {
        hash += key[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }

    return hash;
}

inline uint32_t JenkinsHashEnd(uint64_t hash)
{
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return (uint32_t)hash;
}

inline uint32_t JenkinsHash(const void* data, size_t len)
{
    uint64_t hash = JenkinsHashBegin(data, len);

    return JenkinsHashEnd(hash);
}