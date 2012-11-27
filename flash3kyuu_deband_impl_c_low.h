#include <assert.h>

// freezed code, don't add new feature to it

static __forceinline void __cdecl process_plane_plainc_mode0(const process_plane_params& params, process_plane_context*)
{
    pixel_dither_info* info_ptr;
    unsigned short threshold = params.threshold;
    
    int src_height = params.get_src_height();
    int src_width = params.get_dst_width();

    for (int i = 0; i < src_height; i++)
    {
        const unsigned char* src_px = params.src_plane_ptr + params.src_pitch * i;
        unsigned char* dst_px = params.dst_plane_ptr + params.dst_pitch * i;

        info_ptr = params.info_ptr_base + params.info_stride * i;

        for (int j = 0; j < src_width; j++)
        {
            pixel_dither_info info = *info_ptr;
            assert((abs(info.ref1) >> params.height_subsampling) <= i && (abs(info.ref1) >> params.height_subsampling) + i < params.src_height);

            int ref_pos = (abs(info.ref1) >> params.height_subsampling) * (info.ref1 >> 7) * params.src_pitch;
            int diff = *src_px - src_px[ref_pos];
            if (is_above_threshold(threshold, diff)) {
                *dst_px = *src_px;
            } else {
                *dst_px = src_px[ref_pos];
            }

            src_px++;
            dst_px++;
            info_ptr++;
        }
    }
}

template <int sample_mode, bool blur_first, int mode>
static __forceinline void __cdecl process_plane_plainc_mode12_low(const process_plane_params& params, process_plane_context*)
{
    assert(mode == DA_LOW);

    pixel_dither_info* info_ptr;
    char context[CONTEXT_BUFFER_SIZE];

    unsigned short threshold = params.threshold;

    int pixel_min = params.pixel_min;
    int pixel_max = params.pixel_max;

    int width_subsamp = params.width_subsampling;

    pixel_proc_init_context<mode>(context, params.plane_width_in_pixels, params.output_depth);

    DUMP_INIT("c", params.plane, params.plane_width_in_pixels);

    for (int i = 0; i < params.plane_height_in_pixels; i++)
    {
        const unsigned char* src_px = params.src_plane_ptr + params.src_pitch * i;
        unsigned char* dst_px = params.dst_plane_ptr + params.dst_pitch * i;

        info_ptr = params.info_ptr_base + params.info_stride * i;


        for (int j = 0; j < params.plane_width_in_pixels; j++)
        {
            pixel_dither_info info = *info_ptr;
            int src_px_up = read_pixel<mode>(params, context, src_px);

            DUMP_VALUE("src_px_up", src_px_up);
            
            assert(info.ref1 >= 0);
            assert((info.ref1 >> params.height_subsampling) <= i && 
                   (info.ref1 >> params.height_subsampling) + i < params.plane_height_in_pixels);

            
            DUMP_VALUE("ref1", info.ref1);

            int avg;
            bool use_org_px_as_base;
            int ref_pos, ref_pos_2;
            if (sample_mode == 1)
            {
                ref_pos = (info.ref1 >> params.height_subsampling) * params.src_pitch;
                
                DUMP_VALUE("ref_pos", ref_pos);

                int ref_1_up = read_pixel<mode>(params, context, src_px, ref_pos);
                int ref_2_up = read_pixel<mode>(params, context, src_px, -ref_pos);
                
                DUMP_VALUE("ref_1_up", ref_1_up);
                DUMP_VALUE("ref_2_up", ref_2_up);

                avg = pixel_proc_avg_2<mode>(context, ref_1_up, ref_2_up);

                if (blur_first)
                {
                    int diff = avg - src_px_up;
                    use_org_px_as_base = is_above_threshold(threshold, diff);
                } else {
                    int diff = src_px_up - ref_1_up;
                    int diff_n = src_px_up - ref_2_up;
                    use_org_px_as_base = is_above_threshold(threshold, diff, diff_n);
                }
            } else {
                int x_multiplier = 1;
                
                assert(info.ref2 >= 0);
                assert((info.ref2 >> params.height_subsampling) <= i && 
                       (info.ref2 >> params.height_subsampling) + i < params.plane_height_in_pixels);
                assert(((info.ref1 >> width_subsamp) * x_multiplier) <= j && 
                       ((info.ref1 >> width_subsamp) * x_multiplier) + j < params.plane_width_in_pixels);
                assert(((info.ref2 >> width_subsamp) * x_multiplier) <= j && 
                       ((info.ref2 >> width_subsamp) * x_multiplier) + j < params.plane_width_in_pixels);

                
                DUMP_VALUE("ref2", info.ref2);

                ref_pos = params.src_pitch * (info.ref2 >> params.height_subsampling) + 
                          ((info.ref1 * x_multiplier) >> width_subsamp);

                ref_pos_2 = ((info.ref2 * x_multiplier) >> width_subsamp) - 
                            params.src_pitch * (info.ref1 >> params.height_subsampling);

                
                DUMP_VALUE("ref_pos", ref_pos);
                DUMP_VALUE("ref_pos_2", ref_pos_2);

                int ref_1_up = read_pixel<mode>(params, context, src_px, ref_pos);
                int ref_2_up = read_pixel<mode>(params, context, src_px, ref_pos_2);
                int ref_3_up = read_pixel<mode>(params, context, src_px, -ref_pos);
                int ref_4_up = read_pixel<mode>(params, context, src_px, -ref_pos_2);

                
                DUMP_VALUE("ref_1_up", ref_1_up);
                DUMP_VALUE("ref_2_up", ref_2_up);
                DUMP_VALUE("ref_3_up", ref_3_up);
                DUMP_VALUE("ref_4_up", ref_4_up);

                avg = pixel_proc_avg_4<mode>(context, ref_1_up, ref_2_up, ref_3_up, ref_4_up);

                if (blur_first)
                {
                    int diff = avg - src_px_up;
                    use_org_px_as_base = is_above_threshold(threshold, diff);
                } else {
                    int diff1 = ref_1_up - src_px_up;
                    int diff2 = ref_2_up - src_px_up;
                    int diff3 = ref_3_up - src_px_up;
                    int diff4 = ref_4_up - src_px_up;
                    use_org_px_as_base = is_above_threshold(threshold, diff1, diff2, diff3, diff4);
                }
            }
            
            DUMP_VALUE("avg", avg);
            DUMP_VALUE("change", info.change);

            int new_pixel;

            if (use_org_px_as_base) {
                new_pixel = src_px_up + info.change;
            } else {
                new_pixel = avg + info.change;
            }
            
            DUMP_VALUE("new_pixel_before_downsample", new_pixel);

            new_pixel = pixel_proc_downsample<mode>(context, new_pixel, i, j, pixel_min, pixel_max, params.output_depth);
            
            DUMP_VALUE("dst", new_pixel);

            *dst_px = (unsigned char)new_pixel;

            src_px++;
            dst_px++;
            info_ptr++;
            pixel_proc_next_pixel<mode>(context);
        }
        pixel_proc_next_row<mode>(context);
        DUMP_NEXT_LINE();
    }

    DUMP_FINISH();

    pixel_proc_destroy_context<mode>(context);
}
