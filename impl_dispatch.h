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

const extern process_plane_impl_t process_plane_impl_c[];

const extern process_plane_impl_t process_plane_impl_sse4[];

