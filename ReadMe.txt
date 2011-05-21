flash3kyuu_deband(clip c, int "range", int "Y", int "Cb", int "Cr", 
		int "ditherY", int "ditherC", int "sample_mode", int "seed", 
		int "blur_first", int "diff_seed", int "opt")
		
由 http://www.geocities.jp/flash3kyuu/auf/banding17.zip 移植，此处仅提供简单参数解释，参数详细意义请参考原始aviutl滤镜的文档。
滤镜仅支持逐行YV12源。
		
range
	banding检测范围，默认为15

Y Cb Cr
	banding检测阈值，默认为1
	
ditherY ditherC
	当sample_mode为1~2时有效，模糊后进行dither的强度，默认为1
	
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