#include "stdafx.h"

#include "test.h"


template<int sample_mode, bool blur_first, int precision_mode, int target_impl>
void __cdecl process_plane_correctness_test(const process_plane_params& params, process_plane_context* context)
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
    
    int width = params.dst_vi->RowSize(params.plane);
    int height = params.dst_vi->height >> params.dst_vi->GetPlaneHeightSubsampling(params.plane);

    buffer = create_guarded_buffer(height, params.dst_pitch, plane_start);

    process_plane_params test_params = params;
    test_params.dst_plane_ptr = plane_start;

    printf("First round.\n");

    reference_impl(params, &ref_context);
    test_impl(test_params, &test_context);

    char ref_file_name[256];
    char test_file_name[256];

    sprintf_s(ref_file_name, "correctness_test_reference_%d_%d_%d_%d_%d_%d_%d_%d.bin",
        width, height, params.dst_pitch, params.plane, sample_mode, blur_first, precision_mode, target_impl);
    FILE* ref_file = NULL;
    fopen_s(&ref_file, ref_file_name, "wb");
    if (!ref_file)
    {
        printf(__FUNCTION__ ": Warning: Unable to open %s", ref_file_name);
    }

    sprintf_s(test_file_name, "correctness_test_test_%d_%d_%d_%d_%d_%d_%d_%d.bin",
        width, height, params.dst_pitch, params.plane, sample_mode, blur_first, precision_mode, target_impl);
    FILE* test_file = NULL;
    fopen_s(&test_file, test_file_name, "wb");
    if (!test_file)
    {
        printf(__FUNCTION__ ": Warning: Unable to open %s", test_file_name);
    }

    bool is_correct = true;

    for (int i = 0; i < height; i++)
    {
        unsigned char* ref_start = params.dst_plane_ptr + i * params.dst_pitch;
        unsigned char* test_start = plane_start + i * params.dst_pitch;

        if (ref_file) 
        {
            fwrite(ref_start, 1, width, ref_file);
        }
        if (test_file)
        {
            fwrite(test_start, 1, width, test_file);
        }

        if (memcmp(ref_start, test_start, width) != 0) {
            printf("ERROR(%d, %d, %d, %d): Row %d is different from reference result.\n", sample_mode, blur_first, precision_mode, target_impl, i);
            is_correct = false;
        }
    }

    printf("Second round.\n");

    reference_impl(params, &ref_context);
    test_impl(test_params, &test_context);

    for (int i = 0; i < height; i++)
    {
        unsigned char* ref_start = params.dst_plane_ptr + i * params.dst_pitch;
        unsigned char* test_start = plane_start + i * params.dst_pitch;

        if (ref_file) 
        {
            fwrite(ref_start, 1, width, ref_file);
        }
        if (test_file)
        {
            fwrite(test_start, 1, width, test_file);
        }

        if (memcmp(ref_start, test_start, width) != 0) {
            printf("ERROR(%d, %d, %d, %d): Row %d is different from reference result.\n", sample_mode, blur_first, precision_mode, target_impl, i);
            is_correct = false;
        }
    }

    is_correct = is_correct & check_guard_bytes(buffer, height, params.dst_pitch);
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
    _aligned_free(buffer);

    destroy_context(&ref_context);
    destroy_context(&test_context);
}


#define DECLARE_IMPL_CORRECTNESS_TEST
#include "impl_dispatch_decl.h"
