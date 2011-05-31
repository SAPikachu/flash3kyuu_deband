

namespace pixel_proc_12bit_ordered_dithering {

	#include "pixel_proc_c_12bit_common.h"

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

	static inline int downsample(void* context, int pixel, int row, int column)
	{
		pixel += THRESHOLD_MAP[row & 3][column & 3];
		return pixel >> 4;
	}

};