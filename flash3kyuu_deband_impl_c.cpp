#include "stdafx.h"

// we intend the C version to be usable on most CPUs
#define NO_SSE

#include "flash3kyuu_deband.h"

#include "pixel_proc_c.h"

#include <limits.h>

static inline unsigned char clamp_pixel(int pixel)
{
    if (pixel > 0xff) {
        pixel = 0xff;
    } else if (pixel < 0) {
        pixel = 0;
    }
    return (unsigned char)pixel;
}

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

    for (int i = 0; i < params.src_height; i++)
    {
        const unsigned char* src_px = params.src_plane_ptr + params.src_pitch * i;
        unsigned char* dst_px = params.dst_plane_ptr + params.dst_pitch * i;

        info_ptr = params.info_ptr_base + params.info_stride * i;

        for (int j = 0; j < params.src_width; j++)
        {
            pixel_dither_info info = *info_ptr;
            assert(abs(info.ref1) <= i && abs(info.ref1) + i < params.src_height);

            int ref_pos = info.ref1 * params.src_pitch;
            int diff = *src_px - src_px[ref_pos];
            if (is_above_threshold(params.threshold, diff)) {
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
static __forceinline void __cdecl process_plane_plainc_mode12(const process_plane_params& params, process_plane_context*)
{
    pixel_dither_info* info_ptr;
    char context_y[CONTEXT_BUFFER_SIZE];
    char context_cb[CONTEXT_BUFFER_SIZE];
    char context_cr[CONTEXT_BUFFER_SIZE];

    if (!params.vi->IsYUY2())
    {
        pixel_proc_init_context<mode>(context_y, params.src_width);
    } else {
        pixel_proc_init_context<mode>(context_y, params.src_width / 2);
        pixel_proc_init_context<mode>(context_cb, params.src_width / 4);
        pixel_proc_init_context<mode>(context_cr, params.src_width / 4);
    }
    char* context = context_y;

    for (int i = 0; i < params.src_height; i++)
    {
        const unsigned char* src_px = params.src_plane_ptr + params.src_pitch * i;
        unsigned char* dst_px = params.dst_plane_ptr + params.dst_pitch * i;

        info_ptr = params.info_ptr_base + params.info_stride * i;

        for (int j = 0; j < params.src_width; j++)
        {
            if (params.vi->IsYUY2())
            {
                int index = j & 3;
                switch (index)
                {
                case 0:
                case 2:
                    context = context_y;
                    break;
                case 1:
                    context = context_cb;
                    break;
                case 3:
                    context = context_cr;
                    break;
                default:
                    abort();
                }
            }
            pixel_dither_info info = *info_ptr;
            int src_px_up = pixel_proc_upsample<mode>(context, *src_px);

            assert(abs(info.ref1) <= i && abs(info.ref1) + i < params.src_height);

            int avg;
            bool use_org_px_as_base;
            int ref_pos, ref_pos_2;
            if (sample_mode == 1)
            {
                ref_pos = info.ref1 * params.src_pitch;
                int ref_1_up = pixel_proc_upsample<mode>(context, src_px[ref_pos]);
                int ref_2_up = pixel_proc_upsample<mode>(context, src_px[-ref_pos]);
                avg = pixel_proc_avg_2<mode>(context, ref_1_up, ref_2_up);
                if (blur_first)
                {
                    int diff = avg - src_px_up;
                    use_org_px_as_base = is_above_threshold(params.threshold, diff);
                } else {
                    int diff = src_px_up - ref_1_up;
                    int diff_n = src_px_up - ref_2_up;
                    use_org_px_as_base = is_above_threshold(params.threshold, diff, diff_n);
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

                assert(abs(info.ref2) <= i && abs(info.ref2) + i < params.src_height);
                assert(abs(info.ref1 * x_multiplier) <= j && abs(info.ref1 * x_multiplier) + j < params.src_width);
                assert(abs(info.ref2 * x_multiplier) <= j && abs(info.ref2 * x_multiplier) + j < params.src_width);

                ref_pos = params.src_pitch * info.ref2 + info.ref1 * x_multiplier;
                ref_pos_2 = info.ref2 * x_multiplier - params.src_pitch * info.ref1;

                int ref_1_up = pixel_proc_upsample<mode>(context, src_px[ref_pos]);
                int ref_2_up = pixel_proc_upsample<mode>(context, src_px[ref_pos_2]);
                int ref_3_up = pixel_proc_upsample<mode>(context, src_px[-ref_pos]);
                int ref_4_up = pixel_proc_upsample<mode>(context, src_px[-ref_pos_2]);

                avg = pixel_proc_avg_4<mode>(context, ref_1_up, ref_2_up, ref_3_up, ref_4_up);

                if (blur_first)
                {
                    int diff = avg - src_px_up;
                    use_org_px_as_base = is_above_threshold(params.threshold, diff);
                } else {
                    int diff1 = ref_1_up - src_px_up;
                    int diff2 = ref_2_up - src_px_up;
                    int diff3 = ref_3_up - src_px_up;
                    int diff4 = ref_4_up - src_px_up;
                    use_org_px_as_base = is_above_threshold(params.threshold, diff1, diff2, diff3, diff4);
                }
            }
            int new_pixel;
            if (use_org_px_as_base) {
                new_pixel = src_px_up + info.change;
            } else {
                new_pixel = avg + info.change;
            }
            new_pixel = pixel_proc_downsample<mode>(context, new_pixel, i, j);
            *dst_px = clamp_pixel(new_pixel);

            src_px++;
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
