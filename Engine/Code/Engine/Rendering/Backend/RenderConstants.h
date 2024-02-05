#pragma once

const inline static uint32_t FRAME_COUNT = 3;

#define MAX_RENDER_TARGETS_COUNT 8

#define BIND_OFFSET_INDEX_SRV 0
#define BIND_MAX_INDEX_SRV        (BIND_OFFSET_INDEX_SRV + 1023)

#define BIND_OFFSET_INDEX_SAMPLER (BIND_MAX_INDEX_SRV + 1)
#define BIND_MAX_INDEX_SAPMLER    (BIND_OFFSET_INDEX_SAMPLER + 1023)

#define BIND_OFFSET_INDEX_UAV     (BIND_MAX_INDEX_SAPMLER + 1)
#define BIND_MAX_INDEX_UAV        (BIND_OFFSET_INDEX_UAV + 1023)

#define BIND_OFFSET_INDEX_CBV     (BIND_MAX_INDEX_UAV + 1)
#define BIND_MAX_INDEX_CBV        (BIND_OFFSET_INDEX_CBV + 1023)