#include "stdafx.h"

// we intend the C version to be usable on most CPUs
#define NO_SSE

#include "flash3kyuu_deband.h"

#include "pixel_proc_c.h"

#include <limits.h>

#include "debug_dump.h"

static inline bool _is_above_threshold(int threshold, int diff) {
    return abs(diff) >= threshold;
}

static inline bool is_above_threshold(int threshold, int diff1) {
    return _is_above_threshold(threshold, diff1);
}

static inline bool is_above_threshold(int threshold, int diff1, int diff2) {
    return _is_above_threshold(threshold, diff1) ||
           _is_above_threshold(threshold, diff2);
}

static inline bool is_above_threshold(int threshold, int diff1, int diff2, int diff3, int diff4) {
    return _is_above_threshold(threshold, diff1) ||
           _is_above_threshold(threshold, diff2) ||
           _is_above_threshold(threshold, diff3) ||
           _is_above_threshold(threshold, diff4);
}

static __forceinline void __cdecl process_plane_plainc_mode0(const process_plane_params& params, process_plane_context*)
{
    pixel_dither_info* info_ptr;
    unsigned short threshold = params.threshold;

    for (int i = 0; i < params.src_height; i++)
    {
        const unsigned char* src_px = params.src_plane_ptr + params.src_pitch * i;
        unsigned char* dst_px = params.dst_plane_ptr + params.dst_pitch * i;

        info_ptr = params.info_ptr_base + params.info_stride * i;

        for (int j = 0; j < params.src_width; j++)
        {
            pixel_dither_info info = *info_ptr;
            assert((abs(info.ref1) >> params.height_subsampling) <= i && (abs(info.ref1) >> params.height_subsampling) + i < params.src_height);

            if (params.vi->IsYUY2())
            {
                int index = j & 3;
                switch (index)
                {
                case 0:
                case 2:
                    threshold = params.threshold_y;
                    break;
                case 1:
                    threshold = params.threshold_cb;
                    break;
                case 3:
                    threshold = params.threshold_cr;
                    break;
                default:
                    abort();
                }
            }

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

template <int mode>
static __inline int read_pixel(const process_plane_params& params, void* context, const unsigned char* base, int offset = 0)
{
    const unsigned char* ptr = base + offset;
    if (params.input_mode == LOW_BIT_DEPTH)
    {
        return pixel_proc_upsample<mode>(context, *ptr);
    }

    int ret;

    switch (params.input_mode)
    {
    case HIGH_BIT_DEPTH_STACKED:
        ret = *ptr << 8 | *(ptr + params.plane_height_in_pixels * params.src_pitch);
        break;
    case HIGH_BIT_DEPTH_INTERLEAVED:
        ret = *(unsigned short*)ptr;
        break;
    default:
        // shouldn't happen!
        abort();
        return 0;
    }

    ret <<= (INTERNAL_BIT_DEPTH - params.input_depth);
    return ret;
}

template <int sample_mode, bool blur_first, int mode>
static __forceinline void __cdecl process_plane_plainc_mode12(const process_plane_params& params, process_plane_context*)
{
    pixel_dither_info* info_ptr;
    char context_y[CONTEXT_BUFFER_SIZE];
    char context_cb[CONTEXT_BUFFER_SIZE];
    char context_cr[CONTEXT_BUFFER_SIZE];

    unsigned short threshold = params.threshold;

    int pixel_min = params.pixel_min;
    int pixel_max = params.pixel_max;

    int width_subsamp = params.width_subsampling;

    if (!params.vi->IsYUY2())
    {
        pixel_proc_init_context<mode>(context_y, params.plane_width_in_pixels);
    } else {
        pixel_proc_init_context<mode>(context_y, params.plane_width_in_pixels / 2);
        pixel_proc_init_context<mode>(context_cb, params.plane_width_in_pixels / 4);
        pixel_proc_init_context<mode>(context_cr, params.plane_width_in_pixels / 4);
    }
    char* context = context_y;

    int pixel_step = params.input_mode == HIGH_BIT_DEPTH_INTERLEAVED ? 2 : 1;

    DUMP_INIT("c", params.plane);

    for (int i = 0; i < params.plane_height_in_pixels; i++)
    {
        const unsigned char* src_px = params.src_plane_ptr + params.src_pitch * i;
        unsigned char* dst_px = params.dst_plane_ptr + params.dst_pitch * i;

        info_ptr = params.info_ptr_base + params.info_stride * i;


        for (int j = 0; j < params.plane_width_in_pixels; j++)
        {
            int real_col = j;
            if (params.vi->IsYUY2())
            {
                int index = j & 3;
                switch (index)
                {
                case 0:
                case 2:
                    context = context_y;
                    threshold = params.threshold_y;
                    real_col = j >> 1;
                    pixel_min = params.pixel_min;
                    pixel_max = params.pixel_max;
                    width_subsamp = 0;
                    break;
                case 1:
                    context = context_cb;
                    threshold = params.threshold_cb;
                    real_col = j >> 2;
                    pixel_min = params.pixel_min_c;
                    pixel_max = params.pixel_max_c;
                    width_subsamp = 1;
                    break;
                case 3:
                    context = context_cr;
                    threshold = params.threshold_cr;
                    real_col = j >> 2;
                    pixel_min = params.pixel_min_c;
                    pixel_max = params.pixel_max_c;
                    width_subsamp = 1;
                    break;
                default:
                    abort();
                }
            }
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
                if (params.vi->IsYUY2())
                {
                    if ( (j & 1) == 0 )
                    {
                        // Y
                        x_multiplier = 2;
                    } else {
                        // Cb, Cr
                        x_multiplier = 4;
                    }
                } 
                
                assert(info.ref2 >= 0);
                assert((info.ref2 >> params.height_subsampling) <= i && 
                       (info.ref2 >> params.height_subsampling) + i < params.plane_height_in_pixels);
                assert(((info.ref1 >> width_subsamp) * x_multiplier) <= j && 
                       ((info.ref1 >> width_subsamp) * x_multiplier) + j < params.plane_width_in_pixels);
                assert(((info.ref2 >> width_subsamp) * x_multiplier) <= j && 
                       ((info.ref2 >> width_subsamp) * x_multiplier) + j < params.plane_width_in_pixels);

                
                DUMP_VALUE("ref2", info.ref2);

                ref_pos = params.src_pitch * (info.ref2 >> params.height_subsampling) + 
                          ((info.ref1 * x_multiplier) >> width_subsamp) * pixel_step;

                ref_pos_2 = ((info.ref2 * x_multiplier) >> width_subsamp) * pixel_step - 
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

            new_pixel = pixel_proc_downsample<mode>(context, new_pixel, i, real_col, pixel_min, pixel_max);
            
            DUMP_VALUE("dst", new_pixel);
            switch (mode)
            {
            case PRECISION_LOW:
            case PRECISION_HIGH_NO_DITHERING:
            case PRECISION_HIGH_ORDERED_DITHERING:
            case PRECISION_HIGH_FLOYD_STEINBERG_DITHERING:
                *dst_px = (unsigned char)new_pixel;
                break;
            case PRECISION_16BIT_STACKED:
                *dst_px = (unsigned char)((new_pixel >> 8) & 0xFF);
                *(dst_px + params.plane_height_in_pixels * params.dst_pitch) = (unsigned char)(new_pixel & 0xFF);
                break;
            case PRECISION_16BIT_INTERLEAVED:
                *((unsigned short*)dst_px) = (unsigned short)(new_pixel & 0xFFFF);
                dst_px++;
                break;
            }

            src_px += pixel_step;
            dst_px++;
            info_ptr++;
            pixel_proc_next_pixel<mode>(context);
        }
        pixel_proc_next_row<mode>(context_y);
        if (params.vi->IsYUY2())
        {
            pixel_proc_next_row<mode>(context_cb);
            pixel_proc_next_row<mode>(context_cr);
        }
    }

    DUMP_FINISH();

    pixel_proc_destroy_context<mode>(context_y);
    if (params.vi->IsYUY2())
    {
        pixel_proc_destroy_context<mode>(context_cb);
        pixel_proc_destroy_context<mode>(context_cr);
    }
}

template <int sample_mode, bool blur_first, int mode>
void __cdecl process_plane_plainc(const process_plane_params& params, process_plane_context* context)
{
    if (sample_mode == 0) 
    {
        process_plane_plainc_mode0(params, context);
    } else {
        process_plane_plainc_mode12<sample_mode, blur_first, mode>(params, context);
    }

}

#define DECLARE_IMPL_C
#include "impl_dispatch_decl.h"
