#include "stdafx.h"

#include "test.h"


template<int, bool>
void __cdecl process_plane_sse4_correctness_test(unsigned char const*srcp, int const src_width, int const src_height, int const src_pitch, unsigned char *dstp, int dst_pitch, unsigned char threshold, pixel_dither_info *info_ptr_base, int info_stride, int range)
{
	printf("process_plane_sse4_correctness_test\n");
	printf("-----------------------------------\n");
	process_plane_impl_t reference_impl = process_plane_impl_c[select_impl_index(2, false)];
	process_plane_impl_t test_impl = process_plane_impl_sse4[select_impl_index(2, false)];

	unsigned char* buffer = NULL;
	unsigned char* plane_start = NULL;

	buffer = create_guarded_buffer(src_height, dst_pitch, plane_start);

	DELEGATE_IMPL_CALL(reference_impl, dstp);
	DELEGATE_IMPL_CALL(test_impl, plane_start);

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
	fclose(ref_file);
	fclose(test_file);
	check_guard_bytes(buffer, src_height, dst_pitch);
	_aligned_free(buffer);
	printf("-----------------------------------\n");
}


DEFINE_TEMPLATE_IMPL(sse4_correctness_test, process_plane_sse4_correctness_test);