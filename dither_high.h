
#include "impl_dispatch.h"

#include "pixel_proc_c_high_f_s_dithering.h"

#include "pixel_proc_c_high_ordered_dithering.h"

#include <emmintrin.h>

namespace dither_high
{
	static __m128i _ordered_dithering_threshold_map[4];
	static bool _threshold_map_initialized = false;

	static __inline void init_ordered_dithering()
	{
		if (UNLIKELY(!_threshold_map_initialized)) {
			__m128i threhold_row;
			__m128i zero = _mm_setzero_si128();
			for (int i = 0; i < 4; i++) 
			{
				threhold_row.m128i_u32[0] = *(unsigned int*)pixel_proc_high_ordered_dithering::THRESHOLD_MAP[i];
					
				threhold_row = _mm_unpacklo_epi8(threhold_row, zero);
				threhold_row = _mm_unpacklo_epi64(threhold_row, threhold_row);
				threhold_row = _mm_slli_epi16(threhold_row, pixel_proc_high_ordered_dithering::THRESHOLD_MAP_SHIFT_BITS);

				_mm_store_si128(_ordered_dithering_threshold_map + i, threhold_row);
			}
			_mm_mfence();
			_threshold_map_initialized = true;
		}
	}

	template <int precision_mode>
	static __inline void init(char context_buffer[CONTEXT_BUFFER_SIZE], int frame_width) 
	{
		if (precision_mode == PRECISION_HIGH_FLOYD_STEINBERG_DITHERING)
		{
			pixel_proc_high_f_s_dithering::init_context(context_buffer, frame_width);
		} else if (precision_mode == PRECISION_HIGH_ORDERED_DITHERING) {
			init_ordered_dithering();
		}
	}

	template <int precision_mode>
	static __inline void complete(void* context) 
	{
		if (precision_mode == PRECISION_HIGH_FLOYD_STEINBERG_DITHERING)
		{
			pixel_proc_high_f_s_dithering::destroy_context(context);
		}
	}

	template <int precision_mode>
	static __forceinline __m128i dither(void* context, __m128i pixels, int row, int column)
	{
		switch (precision_mode)
		{
		case PRECISION_LOW:
		case PRECISION_HIGH_NO_DITHERING:
			return pixels;
		case PRECISION_HIGH_ORDERED_DITHERING:
			return _mm_adds_epu16(pixels, _ordered_dithering_threshold_map[row % 4]);
		case PRECISION_HIGH_FLOYD_STEINBERG_DITHERING:
			// due to an unknown ICC bug, access pixels using union will give us incorrect results
			// so we have to use a buffer here
			// tested on ICC 12.0.1024.2010
			__declspec (align(16))
			unsigned short buffer[8];
			_mm_store_si128((__m128i*)buffer, pixels);
			for (int i = 0; i < 8; i++)
			{
				buffer[i] = (unsigned short)pixel_proc_high_f_s_dithering::dither(context, buffer[i], row, column + i);
				pixel_proc_high_f_s_dithering::next_pixel(context);
			}
			return _mm_load_si128((__m128i*)buffer);
		default:
			abort();
			return _mm_setzero_si128();
		}
	}
	
	template <int precision_mode>
	static __inline void next_row(void* context)
	{
		if (precision_mode == PRECISION_HIGH_FLOYD_STEINBERG_DITHERING)
		{
			pixel_proc_high_f_s_dithering::next_row(context);
		}
	}
};