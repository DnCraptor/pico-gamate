#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#define GAMATE_PSRAM_BASE ((uintptr_t)0x11000000u)
#define GAMATE_PSRAM_MAX_SIZE (16u * 1024u * 1024u)

#ifdef __cplusplus
extern "C" {
#endif

bool gamate_psram_init(void);
bool gamate_psram_available(void);
size_t gamate_psram_size(void);
void gamate_psram_reclock(void);

#ifdef __cplusplus
}
#endif
