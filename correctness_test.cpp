#include "stdafx.h"

#include "test.h"


template<int sample_mode, bool blur_first, int target_impl>
void __cdecl process_plane_correctness_test(unsigned char const*srcp, int const src_width, int const src_height, int const src_pitch, unsigned char *dstp, int dst_pitch, unsigned char threshold, pixel_dither_info *info_ptr_base, int info_stride, int range, process_plane_context* context)
{
	printf(__FUNCTION__ ", sample_mode=%d, blur_first=%d, target_impl=%d\n", sample_mode, blur_first, target_impl);
	printf("-----------------------------------\n");
	process_plane_impl_t reference_impl = process_plane_impl_c[select_impl_index(sample_mode, blur_first)];
	process_plane_impl_t test_impl = process_plane_impls[target_impl][select_impl_index(sample_mode, blur_first)];
	
	process_plane_context ref_context;
	process_plane_context test_context;

	init_context(&ref_context);
	init_context(&test_context);

	unsigned char* buffer = NULL;
	unsigned char* plane_start = NULL;

	buffer = create_guarded_buffer(src_height, dst_pitch, plane_start);

	printf("First round.\n");
	
	DELEGATE_IMPL_CALL(reference_impl, dstp, &ref_context);
	DELEGATE_IMPL_CALL(test_impl, plane_start, &test_context);

	char dump_file_name[256];

	sprintf_s(dump_file_name, "correctness_test_reference_%d_%d_%d.bin",src_width, src_height, dst_pitch);
	FILE* ref_file = NULL;
	fopen_s(&ref_file, dump_file_name, "wb");

	sprintf_s(dump_file_name, "correctness_test_test_%d_%d_%d.bin",src_width, src_height, dst_pitch);
	FILE* test_file = NULL;
	fopen_s(&test_file, dump_file_name, "wb");

	for (int i = 0; i < src_height; i++)
	{
		unsigned char* ref_start = dstp + i * dst_pitch;
		unsigned char* test_start = plane_start + i * dst_pitch;
		
		fwrite(ref_start, 1, dst_pitch, ref_file);
		fwrite(test_start, 1, dst_pitch, test_file);

		if (memcmp(ref_start, test_start, src_width) != 0) {
			printf("ERROR: Row %d is different from reference result.\n", i);
		}
	}
	
	printf("Second round.\n");

	DELEGATE_IMPL_CALL(reference_impl, dstp, &ref_context);
	DELEGATE_IMPL_CALL(test_impl, plane_start, &test_context);

	for (int i = 0; i < src_height; i++)
	{
		unsigned char* ref_start = dstp + i * dst_pitch;
		unsigned char* test_start = plane_start + i * dst_pitch;
		
		fwrite(ref_start, 1, dst_pitch, ref_file);
		fwrite(test_start, 1, dst_pitch, test_file);

		if (memcmp(ref_start, test_start, src_width) != 0) {
			printf("ERROR: Row %d is different from reference result.\n", i);
		}
	}


	fclose(ref_file);
	fclose(test_file);
	check_guard_bytes(buffer, src_height, dst_pitch);
	_aligned_free(buffer);

	destroy_context(&ref_context);
	destroy_context(&test_context);
	printf("-----------------------------------\n");
}


DEFINE_TEMPLATE_IMPL_1(correctness_test_sse2, process_plane_correctness_test, IMPL_SSE2);
DEFINE_TEMPLATE_IMPL_1(correctness_test_ssse3, process_plane_correctness_test, IMPL_SSSE3);
DEFINE_TEMPLATE_IMPL_1(correctness_test_sse4, process_plane_correctness_test, IMPL_SSE4);