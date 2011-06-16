#include "stdafx.h"

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

static __forceinline void __cdecl process_plane_plainc_mode0(unsigned char const*srcp, int const src_width, int const src_height, int const src_pitch, unsigned char *dstp, int dst_pitch, unsigned short threshold, pixel_dither_info *info_ptr_base, int info_stride, int range, process_plane_context*)
{
	pixel_dither_info* info_ptr;

	for (int i = 0; i < src_height; i++)
	{
		const unsigned char* src_px = srcp + src_pitch * i;
		unsigned char* dst_px = dstp + dst_pitch * i;

		info_ptr = info_ptr_base + info_stride * i;

		for (int j = 0; j < src_width; j++)
		{
			pixel_dither_info info = *info_ptr;
			assert(abs(info.ref1) <= i && abs(info.ref1) + i < src_height);

			int ref_pos = info.ref1 * src_pitch;
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
static __forceinline void __cdecl process_plane_plainc_mode12(unsigned char const*srcp, int const src_width, int const src_height, int const src_pitch, unsigned char *dstp, int dst_pitch, unsigned short threshold, pixel_dither_info *info_ptr_base, int info_stride, int range, process_plane_context*)
{
	pixel_dither_info* info_ptr;
	char context[CONTEXT_BUFFER_SIZE];
	pixel_proc_init_context<mode>(context, src_width);

	for (int i = 0; i < src_height; i++)
	{
		const unsigned char* src_px = srcp + src_pitch * i;
		unsigned char* dst_px = dstp + dst_pitch * i;

		info_ptr = info_ptr_base + info_stride * i;

		for (int j = 0; j < src_width; j++)
		{
			pixel_dither_info info = *info_ptr;
			int src_px_up = pixel_proc_upsample<mode>(context, *src_px);

			assert(abs(info.ref1) <= i && abs(info.ref1) + i < src_height);

			int avg;
			bool use_org_px_as_base;
			int ref_pos, ref_pos_2;
			if (sample_mode == 1)
			{
				ref_pos = info.ref1 * src_pitch;
				int ref_1_up = pixel_proc_upsample<mode>(context, src_px[ref_pos]);
				int ref_2_up = pixel_proc_upsample<mode>(context, src_px[-ref_pos]);
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
				assert(abs(info.ref1) <= j && abs(info.ref1) + j < src_width);
				assert(abs(info.ref2) <= i && abs(info.ref2) + i < src_height);
				assert(abs(info.ref2) <= j && abs(info.ref2) + j < src_width);

				ref_pos = src_pitch * info.ref2 + info.ref1;
				ref_pos_2 = info.ref2 - src_pitch * info.ref1;

				int ref_1_up = pixel_proc_upsample<mode>(context, src_px[ref_pos]);
				int ref_2_up = pixel_proc_upsample<mode>(context, src_px[ref_pos_2]);
				int ref_3_up = pixel_proc_upsample<mode>(context, src_px[-ref_pos]);
				int ref_4_up = pixel_proc_upsample<mode>(context, src_px[-ref_pos_2]);

				avg = pixel_proc_avg_4<mode>(context, ref_1_up, ref_2_up, ref_3_up, ref_4_up);

				if (blur_first)
				{
					int diff = avg - src_px_up;
					use_org_px_as_base = is_above_threshold(threshold, diff);
				} else {
					int diff1 = ref_1_up - src_px_up;
					int diff2 = ref_3_up - src_px_up;
					int diff3 = ref_2_up - src_px_up;
					int diff4 = ref_4_up - src_px_up;
					use_org_px_as_base = is_above_threshold(threshold, diff1, diff2, diff3, diff4);
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
		pixel_proc_next_row<mode>(context);
	}
	pixel_proc_destroy_context<mode>(context);
}

template <int sample_mode, bool blur_first, int mode>
void __cdecl process_plane_plainc(unsigned char const*srcp, int const src_width, int const src_height, int const src_pitch, unsigned char *dstp, int dst_pitch, unsigned short threshold, pixel_dither_info *info_ptr_base, int info_stride, int range, process_plane_context* context)
{
	if (sample_mode == 0) 
	{
		process_plane_plainc_mode0(srcp, src_width, src_height, src_pitch, dstp, dst_pitch, threshold, info_ptr_base, info_stride, range, context);
	} else {
		process_plane_plainc_mode12<sample_mode, blur_first, mode>(srcp, src_width, src_height, src_pitch, dstp, dst_pitch, threshold, info_ptr_base, info_stride, range, context);
	}

}

#define DECLARE_IMPL_C
#include "impl_dispatch_decl.h"
