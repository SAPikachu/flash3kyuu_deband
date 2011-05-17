#include "stdafx.h"

#include "impl_dispatch.h"

static inline unsigned char sadd8(unsigned char a, int b)
{
    int s = (int)a+b;
    return (unsigned char)(s < 0 ? 0 : s > 0xFF ? 0xFF : s);
}

template <int sample_mode, bool blur_first>
void __cdecl process_plane_plainc(unsigned char const*srcp, int const src_width, int const src_height, int const src_pitch, unsigned char *dstp, int dst_pitch, unsigned char threshold, pixel_dither_info *info_ptr_base, int info_stride, int range)
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
			if (j < range || src_width - j <= range) 
			{
				if (sample_mode == 0) 
				{
					*dst_px = *src_px;
				} else {
					*dst_px = sadd8(*src_px, info.change);
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
						avg = ((int)src_px[ref_px] + (int)src_px[-ref_px]) >> 1;
						if (blur_first)
						{
							int diff = avg - *src_px;
							use_org_px_as_base = IS_ABOVE_THRESHOLD(diff);
						} else {
							int diff = *src_px - src_px[ref_px];
							int diff_n = *src_px - src_px[-ref_px];
							use_org_px_as_base = IS_ABOVE_THRESHOLD(diff) || IS_ABOVE_THRESHOLD(diff_n);
						}
					} else {
						assert(abs(info.ref1) <= j && abs(info.ref1) + j < src_width);
						assert(abs(info.ref2) <= i && abs(info.ref2) + i < src_height);
						assert(abs(info.ref2) <= j && abs(info.ref2) + j < src_width);

						ref_px = src_pitch * info.ref2 + info.ref1;
						ref_px_2 = info.ref2 - src_pitch * info.ref1;

						// consistent with SSE code
						int avg1 = ((int)src_px[ref_px] + (int)src_px[ref_px_2] + 1) >> 1;
						int avg2 = ((int)src_px[-ref_px] + (int)src_px[-ref_px_2] + 1) >> 1;
						avg = (avg1 + avg2) >> 1;

						if (blur_first)
						{
							int diff = avg - *src_px;
							use_org_px_as_base = IS_ABOVE_THRESHOLD(diff);
						} else {
							int diff1 = src_px[ref_px] - *src_px;
							int diff2 = src_px[-ref_px] - *src_px;
							int diff3 = src_px[ref_px_2] - *src_px;
							int diff4 = src_px[-ref_px_2] - *src_px;
							use_org_px_as_base = IS_ABOVE_THRESHOLD(diff1) || 
								IS_ABOVE_THRESHOLD(diff2) ||
								IS_ABOVE_THRESHOLD(diff3) || 
								IS_ABOVE_THRESHOLD(diff4);
						}
					}
					if (use_org_px_as_base) {
						*dst_px = sadd8(*src_px, info.change);
					} else {
						*dst_px = sadd8(avg, info.change);
					}
				}
			}
			src_px++;
			dst_px++;
			info_ptr++;
		}
	}
}

DEFINE_TEMPLATE_IMPL(c, process_plane_plainc);
