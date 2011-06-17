#pragma once

#ifdef IMPL_DISPATCH_IMPORT_DECLARATION

#define DEFINE_IMPL(n, \
					impl_func_mode0, \
					impl_func_mode1_blur, \
					impl_func_mode1_noblur, \
					impl_func_mode2_blur, \
					impl_func_mode2_noblur) \
	extern "C++" const process_plane_impl_t process_plane_impl_##n [];

#else

#define DEFINE_IMPL(n, \
					impl_func_mode0, \
					impl_func_mode1_blur, \
					impl_func_mode1_noblur, \
					impl_func_mode2_blur, \
					impl_func_mode2_noblur) \
	extern "C++" const process_plane_impl_t process_plane_impl_##n [] = { \
					impl_func_mode0, \
					impl_func_mode1_blur, \
					impl_func_mode1_noblur, \
					impl_func_mode2_blur, \
					impl_func_mode2_noblur};

#endif


#define DEFINE_TEMPLATE_IMPL(name, impl_func, ...) \
	DEFINE_IMPL(name, \
				(&impl_func<0, true, __VA_ARGS__>), \
				(&impl_func<1, true, __VA_ARGS__>), \
				(&impl_func<1, false, __VA_ARGS__>), \
				(&impl_func<2, true, __VA_ARGS__>), \
				(&impl_func<2, false, __VA_ARGS__>) );

#define DEFINE_SSE_IMPL(name, ...) \
	DEFINE_TEMPLATE_IMPL(name, process_plane_sse_impl, __VA_ARGS__);


#if defined(IMPL_DISPATCH_IMPORT_DECLARATION) || defined(DECLARE_IMPL_C)
	DEFINE_TEMPLATE_IMPL(c, process_plane_plainc, PIXEL_PROC_8BIT);

	DEFINE_TEMPLATE_IMPL(c_high_no_dithering, process_plane_plainc, PIXEL_PROC_HIGH_NO_DITHERING);
	DEFINE_TEMPLATE_IMPL(c_high_ordered_dithering, process_plane_plainc, PIXEL_PROC_HIGH_ORDERED_DITHERING);
	DEFINE_TEMPLATE_IMPL(c_high_floyd_steinberg_dithering, process_plane_plainc, PIXEL_PROC_HIGH_FLOYD_STEINBERG_DITHERING);
#endif


#if defined(IMPL_DISPATCH_IMPORT_DECLARATION) || defined(DECLARE_IMPL_SSE4)
	DEFINE_SSE_IMPL(sse4, PRECISION_LOW);
	DEFINE_SSE_IMPL(sse4_high_no_dithering, PRECISION_HIGH_NO_DITHERING);
	DEFINE_SSE_IMPL(sse4_high_ordered_dithering, PRECISION_HIGH_ORDERED_DITHERING);
#endif


#if defined(IMPL_DISPATCH_IMPORT_DECLARATION) || defined(DECLARE_IMPL_SSSE3)
	DEFINE_SSE_IMPL(ssse3, PRECISION_LOW);
#endif

	
#if defined(IMPL_DISPATCH_IMPORT_DECLARATION) || defined(DECLARE_IMPL_SSE2)
	DEFINE_SSE_IMPL(sse2, PRECISION_LOW);
#endif
	
#if defined(IMPL_DISPATCH_IMPORT_DECLARATION) || defined(DECLARE_IMPL_BENCHMARK)
	DEFINE_TEMPLATE_IMPL(benchmark, process_plane_benchmark);
#endif

#if defined(IMPL_DISPATCH_IMPORT_DECLARATION) || defined(DECLARE_IMPL_CORRECTNESS_TEST)
	DEFINE_TEMPLATE_IMPL(correctness_test_sse2, process_plane_correctness_test, PRECISION_LOW, IMPL_SSE2);
	DEFINE_TEMPLATE_IMPL(correctness_test_ssse3, process_plane_correctness_test, PRECISION_LOW, IMPL_SSSE3);
	DEFINE_TEMPLATE_IMPL(correctness_test_sse4, process_plane_correctness_test, PRECISION_LOW, IMPL_SSE4);
	
	DEFINE_TEMPLATE_IMPL(correctness_test_sse4_high_no_dithering, process_plane_correctness_test, PRECISION_HIGH_NO_DITHERING, IMPL_SSE4);
	DEFINE_TEMPLATE_IMPL(correctness_test_sse4_high_ordered_dithering, process_plane_correctness_test, PRECISION_HIGH_ORDERED_DITHERING, IMPL_SSE4);
#endif

