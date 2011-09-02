

namespace pixel_proc_16bit {
	
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
		return pixel;
	}

    #define HAS_DOWNSAMPLE

	#include "pixel_proc_c_high_bit_depth_common.h"

	static inline int downsample(void* context, int pixel, int row, int column, int pixel_min, int pixel_max)
	{
        // I know the method name is totally wrong...
		return clamp_pixel(pixel, pixel_min, pixel_max) << (16 - INTERNAL_BIT_DEPTH);
	}


};