#include <assert.h>

namespace pixel_proc_8bit {

    #include "utils.h"

    static inline void init_context(char context_buffer[CONTEXT_BUFFER_SIZE], int frame_width, int output_depth)
    {
        // sanity check only
        assert(output_depth == 8);
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

    static inline int upsample(void* context, unsigned char pixel)
    {
        return pixel;
    }

    static inline int downsample(void* context, int pixel, int row, int column, int pixel_min, int pixel_max, int output_depth)
    {
        assert(output_depth == 8);

        // min and max are in internal depth, we need to scale them to 8bit
        pixel_min >>= (INTERNAL_BIT_DEPTH - 8);
        pixel_max >>= (INTERNAL_BIT_DEPTH - 8);
        
        return clamp_pixel(pixel, pixel_min, pixel_max);
    }

    static inline int avg_2(void* context, int pixel1, int pixel2)
    {
        return (pixel1 + pixel2 + 1) >> 1;
    }

    static inline int avg_4(void* context, int pixel1, int pixel2, int pixel3, int pixel4)
    {
        // consistent with SSE code
        int avg1 = (pixel1 + pixel2 + 1) >> 1;
        int avg2 = (pixel3 + pixel4 + 1) >> 1;
        return (avg1 + avg2) >> 1;
    }

};