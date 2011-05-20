

#include "stdafx.h"

#include "flash3kyuu_deband.h"

#include "impl_dispatch.h"

#include <smmintrin.h>

template <int sample_mode, int ref_part_index>
static __forceinline void process_plane_info_block(
	pixel_dither_info *&info_ptr, 
	__m128i &src_addrs, 
	const __m128i &src_pitch_vector, 
	const __m128i &src_addr_increment_vector, 
	__m128i &change, 
	const __m128i &change_mask, 
	const __m128i &minus_one, 
	unsigned char ref_pixels_1_components[16], 
	unsigned char ref_pixels_2_components[16], 
	unsigned char ref_pixels_3_components[16], 
	unsigned char ref_pixels_4_components[16],
	char*& info_data_stream)
{
	__m128i info_block = _mm_load_si128((__m128i*)info_ptr);

	// change: bit 16-23
	// merge it into the variable, and shuffle it to correct place after all 16 elements are loaded
	__m128i change_temp = info_block;
	change_temp = _mm_and_si128(change_temp, change_mask);

	// this switch should be optimized out
	switch (ref_part_index)
	{
	case 0:
		// right-shift 16 bits
		change_temp = _mm_srli_epi32(change_temp, 16);
		break;
	case 1:
		// right-shift 8 bits
		change_temp = _mm_srli_epi32(change_temp, 8);
		break;
	case 2:
		// already in correct place, do nothing
		break;
	case 3:
		// left-shift 8 bits
		change_temp = _mm_slli_epi32(change_temp, 8);
		break;
	default:
		// CODING ERROR!
		abort();
	}
	
	change = _mm_or_si128(change, change_temp); // combine them

	// ref1: bit 0-7
	// left-shift & right-shift 24bits to remove other elements and preserve sign
	__m128i ref1 = info_block;
	ref1 = _mm_slli_epi32(ref1, 24); // << 24
	ref1 = _mm_srai_epi32(ref1, 24); // >> 24


	// temporarily store computed addresses
	__declspec(align(16))
	unsigned char* address_buffer_1[4];

	__declspec(align(16))
	unsigned char* address_buffer_2[4];

	__m128i ref_offset1;
	__m128i ref_offset2;

	switch (sample_mode)
	{
	case 1:
		ref_offset1 = _mm_mullo_epi32(src_pitch_vector, ref1); // packed DWORD multiplication

		ref_offset2 = _mm_sign_epi32(ref_offset1, minus_one); // negates all offsets
		break;
	case 2:
		// ref2: bit 8-15
		// similar to above
		__m128i ref2;
		ref2 = info_block;
		ref2 = _mm_slli_epi32(ref2, 16); // << 16
		ref2 = _mm_srai_epi32(ref2, 24); // >> 24

		// ref_px = src_pitch * info.ref2 + info.ref1;
		ref_offset1 = _mm_mullo_epi32(src_pitch_vector, ref2); // packed DWORD multiplication
		ref_offset1 = _mm_add_epi32(ref_offset1, ref1);

		// ref_px_2 = info.ref2 - src_pitch * info.ref1;
		ref_offset2 = _mm_mullo_epi32(src_pitch_vector, ref1); // packed DWORD multiplication
		ref_offset2 = _mm_sub_epi32(ref2, ref_offset2);

		break;
	default:
		abort();
	}
	_mm_store_si128((__m128i*)address_buffer_1, _mm_add_epi32(src_addrs, ref_offset1));
	_mm_store_si128((__m128i*)address_buffer_2, _mm_add_epi32(src_addrs, ref_offset2));

	if (info_data_stream){
		_mm_store_si128((__m128i*)info_data_stream, ref_offset1);
		info_data_stream += 16;

		if (sample_mode == 2) {
			_mm_store_si128((__m128i*)info_data_stream, ref_offset2);
			info_data_stream += 16;
		}
	}
	// load ref bytes
	ref_pixels_1_components[4 * ref_part_index + 0] = *(address_buffer_1[0]);
	ref_pixels_1_components[4 * ref_part_index + 1] = *(address_buffer_1[1]);
	ref_pixels_1_components[4 * ref_part_index + 2] = *(address_buffer_1[2]);
	ref_pixels_1_components[4 * ref_part_index + 3] = *(address_buffer_1[3]);

	ref_pixels_2_components[4 * ref_part_index + 0] = *(address_buffer_2[0]);
	ref_pixels_2_components[4 * ref_part_index + 1] = *(address_buffer_2[1]);
	ref_pixels_2_components[4 * ref_part_index + 2] = *(address_buffer_2[2]);
	ref_pixels_2_components[4 * ref_part_index + 3] = *(address_buffer_2[3]);

	if (sample_mode == 2) {

		// another direction, negates all offsets
		ref_offset1 = _mm_sign_epi32(ref_offset1, minus_one);
		_mm_store_si128((__m128i*)address_buffer_1, _mm_add_epi32(src_addrs, ref_offset1));

		ref_offset2 = _mm_sign_epi32(ref_offset2, minus_one);
		_mm_store_si128((__m128i*)address_buffer_2, _mm_add_epi32(src_addrs, ref_offset2));

		ref_pixels_3_components[4 * ref_part_index + 0] = *(address_buffer_1[0]);
		ref_pixels_3_components[4 * ref_part_index + 1] = *(address_buffer_1[1]);
		ref_pixels_3_components[4 * ref_part_index + 2] = *(address_buffer_1[2]);
		ref_pixels_3_components[4 * ref_part_index + 3] = *(address_buffer_1[3]);

		ref_pixels_4_components[4 * ref_part_index + 0] = *(address_buffer_2[0]);
		ref_pixels_4_components[4 * ref_part_index + 1] = *(address_buffer_2[1]);
		ref_pixels_4_components[4 * ref_part_index + 2] = *(address_buffer_2[2]);
		ref_pixels_4_components[4 * ref_part_index + 3] = *(address_buffer_2[3]);

	}
	info_ptr += 4;
	src_addrs = _mm_add_epi32(src_addrs, src_addr_increment_vector);
}

static __inline __m128i clamped_absolute_difference(__m128i a, __m128i b, __m128i difference_limit)
{
	// we need to clamp the result for 2 reasons:
	// 1. there is no integer >= operator in SSE
	// 2. comparison instructions accept only signed integers,
	//    so if difference is bigger than 0x7f, the compare result will be invalid
	__m128i diff = _mm_sub_epi8(_mm_max_epu8(a, b), _mm_min_epu8(a, b));
	return _mm_min_epu8(diff, difference_limit);
}

typedef struct _info_cache
{
	int pitch;
	char* data_stream;
} info_cache;

void destroy_cache(void* data)
{
	assert(data);

	info_cache* cache = (info_cache*) data;
	_aligned_free(cache->data_stream);
	free(data);
}

template<int sample_mode, bool blur_first>
static __m128i __inline process_pixels_mode12(__m128i src_pixels, __m128i threshold_vector, __m128i sign_convert_vector, __m128i one_i8, __m128i change, unsigned char ref_pixels_1_components[16], unsigned char ref_pixels_2_components[16], unsigned char ref_pixels_3_components[16], unsigned char ref_pixels_4_components[16])
{
	__m128i dst_pixels;
	__m128i use_orig_pixel_blend_mask;
	__m128i avg;

	__m128i ref_pixels = _mm_load_si128((__m128i*)ref_pixels_1_components);

	__m128i ref_pixels_2 = _mm_load_si128((__m128i*)ref_pixels_2_components);

	__m128i difference;

	if (!blur_first)
	{
		difference = clamped_absolute_difference(src_pixels, ref_pixels, threshold_vector);
		// mask: if difference >= threshold, set to 0xff, otherwise 0x00
		// difference is already clamped to threshold, so we compare for equality here
		use_orig_pixel_blend_mask = _mm_cmpeq_epi8(difference, threshold_vector);

		difference = clamped_absolute_difference(src_pixels, ref_pixels_2, threshold_vector);
		// use OR to combine the masks
		use_orig_pixel_blend_mask = _mm_or_si128(_mm_cmpeq_epi8(difference, threshold_vector), use_orig_pixel_blend_mask);
	}

	avg = _mm_avg_epu8(ref_pixels, ref_pixels_2);

	if (sample_mode == 2)
	{

		ref_pixels = _mm_load_si128((__m128i*)ref_pixels_3_components);

		ref_pixels_2 = _mm_load_si128((__m128i*)ref_pixels_4_components);

		if (!blur_first)
		{
			difference = clamped_absolute_difference(src_pixels, ref_pixels, threshold_vector);
			use_orig_pixel_blend_mask = _mm_or_si128(_mm_cmpeq_epi8(difference, threshold_vector), use_orig_pixel_blend_mask);

			difference = clamped_absolute_difference(src_pixels, ref_pixels_2, threshold_vector);
			use_orig_pixel_blend_mask = _mm_or_si128(_mm_cmpeq_epi8(difference, threshold_vector), use_orig_pixel_blend_mask);
		}
		// PAVGB adds 1 before calculating average, so we subtract 1 here to be consistent with c version

		__m128i avg2_tmp = _mm_avg_epu8(ref_pixels, ref_pixels_2);
		__m128i avg2 = _mm_min_epu8(avg, avg2_tmp);

		avg = _mm_max_epu8(avg, avg2_tmp);
		avg = _mm_subs_epu8(avg, one_i8);

		avg = _mm_avg_epu8(avg, avg2);

	}

	if (blur_first)
	{
		difference = clamped_absolute_difference(src_pixels, avg, threshold_vector);
		use_orig_pixel_blend_mask = _mm_cmpeq_epi8(difference, threshold_vector);
	}

	// if mask is 0xff (over threshold), select second operand, otherwise select first
	src_pixels = _mm_blendv_epi8(avg, src_pixels, use_orig_pixel_blend_mask);

	// convert to signed form, since change is signed
	src_pixels = _mm_sub_epi8(src_pixels, sign_convert_vector);

	// saturated add
	src_pixels = _mm_adds_epi8(src_pixels, change);

	// convert back to unsigned
	dst_pixels = _mm_add_epi8(src_pixels, sign_convert_vector);
	return dst_pixels;
}

template<int sample_mode, bool blur_first>
static __m128i __inline process_pixels(__m128i src_pixels, __m128i threshold_vector, __m128i sign_convert_vector, __m128i one_i8, __m128i change, unsigned char ref_pixels_1_components[16], unsigned char ref_pixels_2_components[16], unsigned char ref_pixels_3_components[16], unsigned char ref_pixels_4_components[16])
{
	switch (sample_mode)
	{
	case 1:
	case 2:
		return process_pixels_mode12<sample_mode, blur_first>(src_pixels, threshold_vector, sign_convert_vector, one_i8, change, ref_pixels_1_components, ref_pixels_2_components, ref_pixels_3_components, ref_pixels_4_components);
		break;
	default:
		abort();
	}
	return src_pixels;
}


template<int sample_mode, bool blur_first>
void __cdecl process_plane_sse(unsigned char const*srcp, int const src_width, int const src_height, int const src_pitch, unsigned char *dstp, int dst_pitch, unsigned char threshold, pixel_dither_info *info_ptr_base, int info_stride, int range, process_plane_context* context)
{
	// By default, frame buffers are guaranteed to be mod16, and full pitch is always available for every line
	// so even width is not mod16, we don't need to treat remaining pixels as special case
	// see avisynth source code -> core.cpp -> NewPlanarVideoFrame for details

	pixel_dither_info* info_ptr = info_ptr_base;

	// used to compute 4 consecutive addresses
	__m128i src_addr_offset_vector = _mm_set_epi32(3, 2, 1, 0);

	__m128i src_addr_increment_vector = _mm_set1_epi32(4);

	__m128i src_pitch_vector = _mm_set1_epi32(src_pitch);
	
	__m128i change_extract_mask = _mm_set1_epi32(0x00FF0000);

	__m128i change_shuffle_mask = _mm_set_epi32(0x0f0b0703, 0x0e0a0602, 0x0d090501, 0x0c080400);
		
	__m128i threshold_vector = _mm_set1_epi8(threshold);

	__m128i sign_convert_vector = _mm_set1_epi8(0x80u);

	// general-purpose constant
	__m128i minus_one = _mm_set1_epi32(-1);

	__m128i one_i8 = _mm_set1_epi8(1);
	
	bool use_cached_info = false;

	char* info_data_stream = NULL;

	// initialize storage for pre-calculated pixel offsets
	if (context->data) {
		info_cache* cache = (info_cache*) context->data;
		// we need to ensure src_pitch is the same, otherwise offsets will be completely wrong
		// also, if pitch changes, don't waste time to update the cache since it is likely to change again
		if (cache->pitch == src_pitch) {
			info_data_stream = cache->data_stream;
			use_cached_info = true;
		}
	} else {
		// set up buffer for cache
		info_cache* cache = (info_cache*)malloc(sizeof(info_cache));
		info_data_stream = (char*)_aligned_malloc(info_stride * (4 * 2 + 1) * src_height, FRAME_LUT_ALIGNMENT);
		cache->data_stream = info_data_stream;
		cache->pitch = src_pitch;
		context->destroy = destroy_cache;
		context->data = cache;
	}

	for (int row = 0; row < src_height; row++)
	{
		const unsigned char* src_px = srcp + src_pitch * row;
		unsigned char* dst_px = dstp + dst_pitch * row;

		// info_ptr = info_ptr_base + info_stride * row;
		// doesn't need here, since info_stride equals to count of pixels that are needed to process in each row

		int remaining_pixels = info_stride;

		// 4 addresses at a time
		__m128i src_addrs = _mm_set1_epi32((int)src_px);
		// add offset
		src_addrs = _mm_add_epi32(src_addrs, src_addr_offset_vector);

		while (remaining_pixels > 0)
		{
			__m128i change;

			__declspec(align(16))
			unsigned char ref_pixels_1_components[16];
			__declspec(align(16))
			unsigned char ref_pixels_2_components[16];
			__declspec(align(16))
			unsigned char ref_pixels_3_components[16];
			__declspec(align(16))
			unsigned char ref_pixels_4_components[16];

			if (use_cached_info) {
				// cache layout: 16 offset groups (1 or 2 offsets / group depending on sample mode) in a pack, 
				//               followed by 16 bytes of change values
				// in the case of 2 offsets / group, offsets are stored like this:
				// [1 1 1 1 
				//  2 2 2 2
				//  1 1 1 1
				//  2 2 2 2
				//  .. snip
				//  1 1 1 1
				//  2 2 2 2]
				for (int i = 0; i < 16; i++)
				{

					if (sample_mode == 1) {
						ref_pixels_1_components[i] = *(src_px + i + *(int*)(info_data_stream + 4 * i));
						ref_pixels_2_components[i] = *(src_px + i + -*(int*)(info_data_stream + 4 * i));
					}
					else if (sample_mode == 2) {
						ref_pixels_1_components[i] = *(src_px + i + *(int*)(info_data_stream + 4 * (i + i / 4 * 4)));
						ref_pixels_2_components[i] = *(src_px + i + *(int*)(info_data_stream + 4 * (i + i / 4 * 4 + 4)));
						ref_pixels_3_components[i] = *(src_px + i + -*(int*)(info_data_stream + 4 * (i + i / 4 * 4)));
						ref_pixels_4_components[i] = *(src_px + i + -*(int*)(info_data_stream + 4 * (i + i / 4 * 4 + 4)));
					}
				}
				info_data_stream += (sample_mode == 2 ? 128 : 64);
				change = _mm_load_si128((__m128i*)info_data_stream);
				info_data_stream += 16;
			} else {
				// we need to process the info block
				change = _mm_setzero_si128();
			
	#define PROCESS_INFO_BLOCK(n) \
				process_plane_info_block<sample_mode, n>(info_ptr, src_addrs, src_pitch_vector, src_addr_increment_vector, change, change_extract_mask, minus_one, ref_pixels_1_components, ref_pixels_2_components, ref_pixels_3_components, ref_pixels_4_components, info_data_stream);
			
				PROCESS_INFO_BLOCK(0);
				PROCESS_INFO_BLOCK(1);
				PROCESS_INFO_BLOCK(2);
				PROCESS_INFO_BLOCK(3);

	#undef PROCESS_INFO_BLOCK

				// shuffle delta values to correct place
				change = _mm_shuffle_epi8(change, change_shuffle_mask);

				if (info_data_stream) {
					_mm_store_si128((__m128i*)info_data_stream, change);
					info_data_stream += 16;
				}

			}

			__m128i src_pixels = _mm_load_si128((__m128i*)src_px);
			__m128i dst_pixels = process_pixels<sample_mode, blur_first>(src_pixels, threshold_vector, sign_convert_vector, one_i8, change, ref_pixels_1_components, ref_pixels_2_components, ref_pixels_3_components, ref_pixels_4_components);
			// write back the result
			_mm_store_si128((__m128i*)dst_px, dst_pixels);

			src_px += 16;
			dst_px += 16;
			remaining_pixels -= 16;
		}
	}
}

DEFINE_TEMPLATE_IMPL(sse4, process_plane_sse);
