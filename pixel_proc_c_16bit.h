

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
	static inline int downsample(void* context, int pixel, int row, int column)
	{
        // I know the method name is totally wrong...
		return pixel << (16 - INTERNAL_BIT_DEPTH);
	}

	#include "pixel_proc_c_high_bit_depth_common.h"

};