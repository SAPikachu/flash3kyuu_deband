#include "stdafx.h"

#include "test.h"

#define BENCHMARK_ROUNDS 10

template <class T, size_t element_count>
T avg(T (&elems)[element_count])
{
	T total = 0;
	for (int i = 0; i < element_count; i++)
	{
		total += elems[i];
	}
	return total / element_count;
}

template<int sample_mode, bool blur_first>
void __cdecl process_plane_benchmark(unsigned char const*srcp, int const src_width, int const src_height, int const src_pitch, unsigned char *dstp, int dst_pitch, unsigned char threshold, pixel_dither_info *info_ptr_base, int info_stride, int range, process_plane_context* context)
{
	HANDLE process_handle = GetCurrentProcess();
	HANDLE thread_handle = GetCurrentThread();

	DWORD old_process_priority = GetPriorityClass(process_handle);
	DWORD old_thread_priority = GetThreadPriority(thread_handle);

	SetPriorityClass(process_handle, HIGH_PRIORITY_CLASS);
	SetThreadPriority(thread_handle, THREAD_PRIORITY_HIGHEST);

	DWORD_PTR old_affinity = SetThreadAffinityMask(thread_handle, (DWORD_PTR)1);
	
	printf(__FUNCTION__ ", sample_mode=%d, blur_first=%d\n", sample_mode, blur_first);
	printf("-----------------------------------\n");

	int total_bytes = src_width * src_height;
	printf("Width: %d, Height: %d, Total: %d bytes\n", src_width, src_height, total_bytes);

	process_plane_impl_t c_impl = process_plane_impls[PRECISION_LOW][IMPL_C][select_impl_index(sample_mode, blur_first)];
	process_plane_impl_t sse_impl = process_plane_impls[PRECISION_LOW][IMPL_SSE4][select_impl_index(sample_mode, blur_first)];

	DWORD64 tsc_before, tsc_after;
	DWORD64 cycles_elapsed;
	double cycles_per_byte;

#define TIMED_RUN(impl) do { READ_TSC(tsc_before); \
							 DELEGATE_IMPL_CALL(impl, dstp, context); \
							 READ_TSC(tsc_after); \
							 cycles_elapsed = tsc_after - tsc_before; \
							 cycles_per_byte = (double)cycles_elapsed / total_bytes; \
						} while (0)

	printf("Warm up: \n");
	
	TIMED_RUN(c_impl);
	printf("C: %lld cycles (%.2f cycles per byte)\n", cycles_elapsed, cycles_per_byte);
	TIMED_RUN(sse_impl);
	printf("SSE: %lld cycles (%.2f cycles per byte)\n", cycles_elapsed, cycles_per_byte);

	printf("\n");

	printf("Testing for %d rounds...\n", BENCHMARK_ROUNDS);

#define BENCHMARK(impl, name) do { \
		DWORD64 results[BENCHMARK_ROUNDS]; \
		TIMED_RUN(impl); \
		for (int i = 0; i < BENCHMARK_ROUNDS; i++) \
		{ \
			TIMED_RUN(impl); \
			results[i] = cycles_elapsed; \
		} \
		printf("Result for " name ":\n"); \
		printf("Total cycles:\n"); \
		for (int i = 0; i < BENCHMARK_ROUNDS; i++) \
		{ \
			printf("%-15lld ", results[i]); \
		} \
		printf("\n"); \
		printf("Cycles per byte:\n"); \
		for (int i = 0; i < BENCHMARK_ROUNDS; i++) \
 		{ \
			printf("%-15.2f ", (double)results[i] / total_bytes); \
		} \
		printf("\n"); \
		DWORD64 avg_cycles = avg(results); \
		printf("Average cycles: %lld (%.2f cycles per byte)\n", avg_cycles, (double)avg_cycles / total_bytes); \
		printf("\n\n"); \
	} while (0)
		
	BENCHMARK(c_impl, "C");
	BENCHMARK(sse_impl, "SSE");
	

	printf("-----------------------------------\n");

	SetThreadAffinityMask(thread_handle, old_affinity);
	SetPriorityClass(process_handle, old_process_priority);
	SetThreadPriority(thread_handle, old_thread_priority);
}

#define DECLARE_IMPL_BENCHMARK
#include "impl_dispatch_decl.h"
