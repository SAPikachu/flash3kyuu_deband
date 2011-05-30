#include "stdafx.h"

#include "impl_dispatch.h"

#include "pixel_proc_c.h"

template <int sample_mode, bool blur_first, int mode>
void __cdecl process_plane_plainc(unsigned char const*srcp, int const src_width, int const src_height, int const src_pitch, unsigned char *dstp, int dst_pitch, unsigned char threshold, pixel_dither_info *info_ptr_base, int info_stride, int range, process_plane_context*)
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

			if (j < range || src_width - j <= range) 
			{
				if (sample_mode == 0) 
				{
					*dst_px = *src_px;
				} else {
					*dst_px = pixel_proc_downsample<mode>(context, src_px_up + info.change);
				}
			} else {
#define IS_ABOVE_THRESHOLD(diff) ( (diff ^ (diff >> 31)) - (diff >> 31) >= threshold )

				assert(abs(info.ref1) <= i && abs(info.ref1) + i < src_height);

				if (sample_mode == 0) 
				{
					int ref_px = info.ref1 * src_pitch;
					int diff = *src_px - src_px[ref_px];
					if (IS_ABOVE_THRESHOLD(diff)) {
						*dst_px = *src_px;
					} else {
						*dst_px = src_px[ref_px];
					}
				} else {
					int avg;
					bool use_org_px_as_base;
					int ref_px, ref_px_2;
					if (sample_mode == 1)
					{
						ref_px = info.ref1 * src_pitch;
						int ref_1_up = pixel_proc_upsample<mode>(context, src_px[ref_px]);
						int ref_2_up = pixel_proc_upsample<mode>(context, src_px[-ref_px]);
						avg = pixel_proc_avg_2<mode>(context, ref_1_up, ref_2_up);
						if (blur_first)
						{
							int diff = avg - src_px_up;
							use_org_px_as_base = IS_ABOVE_THRESHOLD(diff);
						} else {
							int diff = src_px_up - ref_1_up;
							int diff_n = src_px_up - ref_2_up;
							use_org_px_as_base = IS_ABOVE_THRESHOLD(diff) || IS_ABOVE_THRESHOLD(diff_n);
						}
					} else {
						assert(abs(info.ref1) <= j && abs(info.ref1) + j < src_width);
						assert(abs(info.ref2) <= i && abs(info.ref2) + i < src_height);
						assert(abs(info.ref2) <= j && abs(info.ref2) + j < src_width);

						ref_px = src_pitch * info.ref2 + info.ref1;
						ref_px_2 = info.ref2 - src_pitch * info.ref1;

						int ref_1_up = pixel_proc_upsample<mode>(context, src_px[ref_px]);
						int ref_2_up = pixel_proc_upsample<mode>(context, src_px[ref_px_2]);
						int ref_3_up = pixel_proc_upsample<mode>(context, src_px[-ref_px]);
						int ref_4_up = pixel_proc_upsample<mode>(context, src_px[-ref_px_2]);

						avg = pixel_proc_avg_4<mode>(context, ref_1_up, ref_2_up, ref_3_up, ref_4_up);

						if (blur_first)
						{
							int diff = avg - src_px_up;
							use_org_px_as_base = IS_ABOVE_THRESHOLD(diff);
						} else {
							int diff1 = ref_1_up - src_px_up;
							int diff2 = ref_3_up - src_px_up;
							int diff3 = ref_2_up - src_px_up;
							int diff4 = ref_4_up - src_px_up;
							use_org_px_as_base = 
								IS_ABOVE_THRESHOLD(diff1) || 
								IS_ABOVE_THRESHOLD(diff2) ||
								IS_ABOVE_THRESHOLD(diff3) || 
								IS_ABOVE_THRESHOLD(diff4);
						}
					}
					int new_pixel;
					if (use_org_px_as_base) {
						new_pixel = src_px_up + info.change;
					} else {
						new_pixel = avg + info.change;
					}
					*dst_px = pixel_proc_downsample<mode>(context, new_pixel);
				}
			}
			src_px++;
			dst_px++;
			info_ptr++;
			pixel_proc_next_pixel<mode>(context);
		}
		pixel_proc_next_row<mode>(context);
	}
	pixel_proc_destroy_context<mode>(context);
}

DEFINE_TEMPLATE_IMPL_1(c, process_plane_plainc, PIXEL_PROC_8BIT);
