
    static inline unsigned char clamp_pixel(int pixel)
    {
        if (pixel > 0xff) {
            pixel = 0xff;
        } else if (pixel < 0) {
            pixel = 0;
        }
        return (unsigned char)pixel;
    }



	static inline int upsample(void* context, unsigned char pixel)
	{
		return pixel << (BIT_DEPTH - 8);
	}

#if defined(HAS_DOWNSAMPLE)
#undef HAS_DOWNSAMPLE
#else
	static inline int downsample(void* context, int pixel, int row, int column)
	{
		pixel = dither(context, pixel, row, column);
		return clamp_pixel(pixel >> (BIT_DEPTH - 8));
	}
#endif

	static inline int avg_2(void* context, int pixel1, int pixel2)
	{
		return (pixel1 + pixel2) >> 1;
	}

	static inline int avg_4(void* context, int pixel1, int pixel2, int pixel3, int pixel4)
	{
		return (pixel1 + pixel2 + pixel3 + pixel4) >> 2;
	}

