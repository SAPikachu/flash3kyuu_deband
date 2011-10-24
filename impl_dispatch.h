#pragma once

#include "flash3kyuu_deband.h"

extern const process_plane_impl_t** process_plane_impls[];

#define IMPL_C 0
#define IMPL_SSE2 1
#define IMPL_SSSE3 2
#define IMPL_SSE4 3

#define IMPL_COUNT 8

#define DITHER_CONTEXT_BUFFER_SIZE 8192

#define CONTEXT_BUFFER_SIZE DITHER_CONTEXT_BUFFER_SIZE

static __inline int select_impl_index(int sample_mode, bool blur_first)
{
	if (sample_mode == 0)
	{
		return 0;
	}
	return sample_mode * 2 + (blur_first ? 0 : 1) - 1;
}
