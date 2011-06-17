

namespace pixel_proc_high_ordered_dithering {
	
	static const int BIT_DEPTH = 14;

	static const int THRESHOLD_MAP_SHIFT_BITS = (BIT_DEPTH - 12);


	// from https://secure.wikimedia.org/wikipedia/en/wiki/Ordered_dithering
	static const unsigned char THRESHOLD_MAP [4] [4] =
	{
		{ 0,  8,  2, 10},
		{12,  4, 14,  6},
		{ 3, 11,  1,  9},
		{15,  7, 13,  5}
	};


	static inline void init_context(char context_buffer[CONTEXT_BUFFER_SIZE], int frame_width)
	{
		// nothing to do
	}

	static inline void destroy_context(void* context)
	{
		// nothing to do
	}

	static inline void next_pixel(void* context)
	{
		// nothing to do
	}

	static inline void next_row(void* context)
	{
		// nothing to do
	}

	static inline int dither(void* context, int pixel, int row, int column)
	{
		pixel += (THRESHOLD_MAP[row & 3][column & 3] << THRESHOLD_MAP_SHIFT_BITS );
		return pixel;
	}

	#include "pixel_proc_c_high_bit_depth_common.h"
};