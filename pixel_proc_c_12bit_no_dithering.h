

namespace pixel_proc_12bit_no_dithering {

	#include "pixel_proc_c_12bit_common.h"

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

	static inline int downsample(void* context, int pixel)
	{
		return pixel >> 4;
	}

};