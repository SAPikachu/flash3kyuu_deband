flash3kyuu_deband(clip c, int "range", int "Y", int "Cb", int "Cr", 
		int "ditherY", int "ditherC", int "sample_mode", int "seed", 
		int "blur_first", int "diff_seed")
		
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