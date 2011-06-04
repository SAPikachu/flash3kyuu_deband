#pragma once

#include "flash3kyuu_deband.h"

extern const process_plane_impl_t** process_plane_impls[];

#define PRECISION_LOW 0
#define PRECISION_HIGH_NO_DITHERING 1
#define PRECISION_HIGH_ORDERED_DITHERING 2
#define PRECISION_HIGH_FLOYD_STEINBERG_DITHERING 3

#define PRECISION_COUNT 4


#define IMPL_C 0
#define IMPL_SSE2 1
#define IMPL_SSSE3 2
#define IMPL_SSE4 3

#define IMPL_COUNT 8

static __inline int select_impl_index(int sample_mode, bool blur_first)
{
	if (sample_mode == 0)
	{
		return 0;
	}
	return sample_mode * 2 + (blur_first ? 0 : 1) - 1;
}
