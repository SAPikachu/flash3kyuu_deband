#include "stdafx.h"

#include "test.h"


template<int sample_mode, bool blur_first, int precision_mode, int target_impl>
void __cdecl process_plane_correctness_test(unsigned char const*srcp, int const src_width, int const src_height, int const src_pitch, unsigned char *dstp, int dst_pitch, unsigned short threshold, pixel_dither_info *info_ptr_base, int info_stride, int range, process_plane_context* context)
{
	printf("\r" __FUNCTION__ ", s_mode=%d, blur=%d, precision=%d, target_impl=%d \n", 
		sample_mode, blur_first, precision_mode, target_impl);

	process_plane_impl_t reference_impl = process_plane_impls[precision_mode][IMPL_C][select_impl_index(sample_mode, blur_first)];
	process_plane_impl_t test_impl = process_plane_impls[precision_mode][target_impl][select_impl_index(sample_mode, blur_first)];
	
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
	
	char ref_file_name[256];
	char test_file_name[256];

	sprintf_s(ref_file_name, "correctness_test_reference_%d_%d_%d_%d_%d_%d_%d.bin",
		src_width, src_height, dst_pitch, sample_mode, blur_first, precision_mode, target_impl);
	FILE* ref_file = NULL;
	fopen_s(&ref_file, ref_file_name, "wb");
	if (!ref_file)
	{
		printf(__FUNCTION__ ": Warning: Unable to open %s", ref_file_name);
	}

	sprintf_s(test_file_name, "correctness_test_test_%d_%d_%d_%d_%d_%d_%d.bin",
		src_width, src_height, dst_pitch, sample_mode, blur_first, precision_mode, target_impl);
	FILE* test_file = NULL;
	fopen_s(&test_file, test_file_name, "wb");
	if (!test_file)
	{
		printf(__FUNCTION__ ": Warning: Unable to open %s", test_file_name);
	}

	bool is_correct = true;

	for (int i = 0; i < src_height; i++)
	{
		unsigned char* ref_start = dstp + i * dst_pitch;
		unsigned char* test_start = plane_start + i * dst_pitch;
		
		if (ref_file) 
		{
			fwrite(ref_start, 1, dst_pitch, ref_file);
		}
		if (test_file)
		{
			fwrite(test_start, 1, dst_pitch, test_file);
		}

		if (memcmp(ref_start, test_start, src_width) != 0) {
			printf("ERROR(%d, %d, %d, %d): Row %d is different from reference result.\n", sample_mode, blur_first, precision_mode, target_impl, i);
			is_correct = false;
		}
	}
	
	printf("Second round.\n");

	DELEGATE_IMPL_CALL(reference_impl, dstp, &ref_context);
	DELEGATE_IMPL_CALL(test_impl, plane_start, &test_context);

	for (int i = 0; i < src_height; i++)
	{
		unsigned char* ref_start = dstp + i * dst_pitch;
		unsigned char* test_start = plane_start + i * dst_pitch;
		
		if (ref_file) 
		{
			fwrite(ref_start, 1, dst_pitch, ref_file);
		}
		if (test_file)
		{
			fwrite(test_start, 1, dst_pitch, test_file);
		}

		if (memcmp(ref_start, test_start, src_width) != 0) {
			printf("ERROR(%d, %d, %d, %d): Row %d is different from reference result.\n", sample_mode, blur_first, precision_mode, target_impl, i);
			is_correct = false;
		}
	}

	if (ref_file)
	{
		fclose(ref_file);
		if (is_correct) 
		{
			remove(ref_file_name);
		}
	}
	if (test_file)
	{
		fclose(test_file);
		if (is_correct) 
		{
			remove(test_file_name);
		}
	}
	check_guard_bytes(buffer, src_height, dst_pitch);
	_aligned_free(buffer);

	destroy_context(&ref_context);
	destroy_context(&test_context);
}


#define DECLARE_IMPL_CORRECTNESS_TEST
#include "impl_dispatch_decl.h"
