#pragma once

#include "flash3kyuu_deband.h"

#define DEFINE_IMPL(n, \
					impl_func_mode0, \
					impl_func_mode1_blur, \
					impl_func_mode1_noblur, \
					impl_func_mode2_blur, \
					impl_func_mode2_noblur) \
	const process_plane_impl_t process_plane_impl_##n [] = { \
					impl_func_mode0, \
					impl_func_mode1_blur, \
					impl_func_mode1_noblur, \
					impl_func_mode2_blur, \
					impl_func_mode2_noblur};


#define DEFINE_TEMPLATE_IMPL(name, impl_func) \
	DEFINE_IMPL(name, \
				(&impl_func<0, true>), \
				(&impl_func<1, true>), \
				(&impl_func<1, false>), \
				(&impl_func<2, true>), \
				(&impl_func<2, false>) );

#define DEFINE_TEMPLATE_IMPL_1(name, impl_func, param) \
	DEFINE_IMPL(name, \
				(&impl_func<0, true, param>), \
				(&impl_func<1, true, param>), \
				(&impl_func<1, false, param>), \
				(&impl_func<2, true, param>), \
				(&impl_func<2, false, param>) );

const extern process_plane_impl_t process_plane_impl_c[];

const extern process_plane_impl_t process_plane_impl_sse4[];

const extern process_plane_impl_t process_plane_impl_ssse3[];

const extern process_plane_impl_t process_plane_impl_correctness_test_sse2[];

const extern process_plane_impl_t process_plane_impl_correctness_test_ssse3[];

const extern process_plane_impl_t process_plane_impl_correctness_test_sse4[];

const extern process_plane_impl_t process_plane_impl_benchmark[];

static const process_plane_impl_t* process_plane_impls[] = {
	process_plane_impl_c,
	NULL,
	process_plane_impl_ssse3,
	process_plane_impl_sse4,
	process_plane_impl_correctness_test_sse2,
	process_plane_impl_correctness_test_ssse3,
	process_plane_impl_correctness_test_sse4,
	process_plane_impl_benchmark
};

#define IMPL_C 0
#define IMPL_SSE2 1
#define IMPL_SSSE3 2
#define IMPL_SSE4 3

static __inline int select_impl_index(int sample_mode, bool blur_first)
{
	if (sample_mode == 0)
	{
		return 0;
	}
	return sample_mode * 2 + (blur_first ? 0 : 1) - 1;
}
