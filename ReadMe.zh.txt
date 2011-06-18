flash3kyuu_deband(clip c, int "range", int "Y", int "Cb", int "Cr", 
		int "ditherY", int "ditherC", int "sample_mode", int "seed", 
		bool "blur_first", bool "diff_seed", int "opt", bool "mt", 
		int "precision_mode")
		
由 http://www.geocities.jp/flash3kyuu/auf/banding17.zip 移植，此处仅提供简单参数解释，参数详细意义请参考原始aviutl滤镜的文档。
滤镜仅支持逐行YV12源。
		
range
	banding检测范围，默认为15

Y Cb Cr
	banding检测阈值，如设为0，则对应的平面完全不作任何处理。
	
	默认：1 (precision_mode = 0) /
	      64 (precision_mode > 0)
	
ditherY ditherC
	当sample_mode为1~2时有效，模糊后进行dither的强度。
	
	默认：1 (precision_mode = 0) /
	      64 (precision_mode > 0)
	      
sample_mode
	0：不进行模糊，处理时保持原像素值
	1：使用2个样本进行模糊处理
	2：使用4个样本进行模糊处理
	
	默认为1
	
seed
	随机数种子，一般不需要设置
	
blur_first
	当sample_mode为1~2时有效，是否在检测前进行模糊，默认为启用
	
diff_seed
	每帧均使用不同的随机数种子，默认为不使用
	
opt
	优化模式
	-1：使用当前CPU最高支持的优化
	0：纯C代码
	1：SSE2
	2：SSSE3
	3：SSE4.1
	
	默认为-1，一般不需要更改，i5-520m测试SSE模式比C快60%~100%（视模式而定）。
	
mt
	多线程模式。如果设为true，U及V平面会在新线程与Y平面同时处理。
	
	如用户机器有多个CPU/核心则默认启用，否则默认为禁用
	
precision_mode
	0: 低精度模式
	1: 高精度模式, 无dither处理
	2: 高精度模式, Ordered dithering
	3: 高精度模式, Floyd-Steinberg dithering
	
	说明：
	#1 该参数仅在sample_mode > 0时有效，sample_mode = 0时设置该参数会出错。
	#2 不推荐模式0（除非sample_mode为0），需要高速处理的话建议使用模式1或2，否则建议质量较好的模式3。
	#3 要达到同等强度，高精度模式的阈值及dither设置值为低精度模式下的64倍。
	
	默认值: 0 (sample_mode = 0) / 3 (sample_mode > 0)