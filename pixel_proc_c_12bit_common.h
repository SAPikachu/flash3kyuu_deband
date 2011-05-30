
	static inline int upsample(unsigned char pixel)
	{
		return pixel << 4;
	}

	static inline int avg_2(int pixel1, int pixel2)
	{
		return (pixel1 + pixel2) >> 1;
	}

	static inline int avg_4(int pixel1, int pixel2, int pixel3, int pixel4)
	{
		return (pixel1 + pixel2 + pixel3 + pixel4) >> 2;
	}

