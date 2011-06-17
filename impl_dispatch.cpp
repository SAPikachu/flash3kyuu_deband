#include "stdafx.h"

#include "flash3kyuu_deband.h"

#define IMPL_DISPATCH_IMPORT_DECLARATION

#include "impl_dispatch_decl.h"

const process_plane_impl_t* process_plane_impl_low_precision[] = {
	process_plane_impl_c,
	process_plane_impl_sse2,
	process_plane_impl_ssse3,
	process_plane_impl_sse4,

	process_plane_impl_correctness_test_sse2,
	process_plane_impl_correctness_test_ssse3,
	process_plane_impl_correctness_test_sse4,
	process_plane_impl_benchmark,
};

const process_plane_impl_t* process_plane_impl_high_precision_no_dithering[] = {
	process_plane_impl_c_high_no_dithering,
	process_plane_impl_c_high_no_dithering,
	process_plane_impl_c_high_no_dithering,
	process_plane_impl_sse4_high_no_dithering,
	process_plane_impl_c_high_no_dithering,
	process_plane_impl_c_high_no_dithering,
	process_plane_impl_correctness_test_sse4_high_no_dithering,
	process_plane_impl_c_high_no_dithering,
};

const process_plane_impl_t* process_plane_impl_high_precision_ordered_dithering[] = {
	process_plane_impl_c_high_ordered_dithering,
	process_plane_impl_c_high_ordered_dithering,
	process_plane_impl_c_high_ordered_dithering,
	process_plane_impl_sse4_high_ordered_dithering,
	process_plane_impl_c_high_ordered_dithering,
	process_plane_impl_c_high_ordered_dithering,
	process_plane_impl_correctness_test_sse4_high_ordered_dithering,
	process_plane_impl_c_high_ordered_dithering,
};

const process_plane_impl_t* process_plane_impl_high_precision_floyd_steinberg_dithering[] = {
	process_plane_impl_c_high_floyd_steinberg_dithering,
	process_plane_impl_c_high_floyd_steinberg_dithering,
	process_plane_impl_c_high_floyd_steinberg_dithering,
	process_plane_impl_c_high_floyd_steinberg_dithering,
	process_plane_impl_c_high_floyd_steinberg_dithering,
	process_plane_impl_c_high_floyd_steinberg_dithering,
	process_plane_impl_c_high_floyd_steinberg_dithering,
	process_plane_impl_c_high_floyd_steinberg_dithering,
};


const process_plane_impl_t** process_plane_impls[] = {
	process_plane_impl_low_precision,
	process_plane_impl_high_precision_no_dithering,
	process_plane_impl_high_precision_ordered_dithering,
	process_plane_impl_high_precision_floyd_steinberg_dithering,
};