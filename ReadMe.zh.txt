f3kdb(clip c, int "range", int "Y", int "Cb", int "Cr", 
		int "ditherY", int "ditherC", int "sample_mode", int "seed", 
		bool "blur_first", bool "dynamic_dither_noise", int "opt", bool "mt", 
		int "precision_mode", bool "keep_tv_range", int "input_mode",
		int "input_depth", int "output_mode", int "output_depth", 
		bool "enable_fast_skip_plane", int "random_algo_ref",
		int "random_algo_dither")
		
由 http://www.geocities.jp/flash3kyuu/auf/banding17.zip 移植，此处仅提供简单参数解释，参数详细意义请参考原始aviutl滤镜的文档。
滤镜支持逐行YUY2、YV12、YV16、YV24及YV411。
		
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
	
	默认为2
	
seed
	随机数种子，一般不需要设置
	
blur_first
	当sample_mode为1~2时有效，是否在检测前进行模糊，默认为启用
	
dynamic_dither_noise
	动态噪声模式，默认为不使用
	
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
	0：低精度模式
	1：高精度模式, 无dither处理
	2：高精度模式, Ordered dithering
	3：高精度模式, Floyd-Steinberg dithering
	4：高精度模式, 16bit层叠输出，供dither工具集后续处理（http://forum.doom9.org/showthread.php?p=1386559#post1386559）
	5：高精度模式, 16bit交织输出，供10bit x264编码 （此模式下帧宽度会变成原始的2倍，并且预览时会花屏。正确压制后就会恢复正常。）
	
	说明：
	#1 该参数仅在sample_mode > 0时有效，sample_mode = 0时设置该参数会出错。
	#2 不推荐模式0（除非sample_mode为0），需要高速处理的话建议使用模式1或2，否则建议质量较好的模式3。
	#3 要达到同等强度，高精度模式的阈值及dither设置值为低精度模式下的64倍。
	#4 10bit x264命令行范例：
	   avs2yuv -raw "script.avs" -o - | x264-10bit --demuxer raw --input-depth 16 --input-res 1280x720 --fps 24 --output "out.mp4" -
	   
	   或使用添加过hack（ https://gist.github.com/1117711 ）的x264编译版：
	   x264-10bit --input-depth 16 --output "out.mp4" script.avs
	
	默认值：0 (sample_mode = 0) / 3 (sample_mode > 0)
	
keep_tv_range
	如设置为true，所有处理后的像素会被裁剪到TV range (luma：16 ~ 235, 
	chroma：16 ~ 240)。
	
	如果视频源是TV range，推荐设置为true，因为dither后的像素有可能会超出范围。
	
	如果视频源是full-range，*不要*设置此参数为true，否则所有超出TV range的像素会被裁剪。
	
	默认值：false
	
	
input_mode
	指定输入视频模式。
	
	0：标准8bit视频
	1：9 ~ 16 bit 高位深视频, 层叠格式
	2：9 ~ 16 bit 高位深视频, 交织格式
	
	默认值：0
	
input_depth
	指定源视频位深。
	
	范围：8 ~ 16
	默认值：8 (input_mode = 0) / 16 (input_mode = 1 / 2)
	
output_mode
	指定输出视频模式。参数值意义同input_mode。
	
	仅当 precision_mode = 1 / 2 / 3 时可用。
	
	当 precision_mode = 0 / 4 / 5 ，output_mode会自动设定为对应值且不能被更改。
	
	默认值：当precision_mode = 0 ~ 3 且 output_depth 未设置时为0， 
	        否则自动选择为最合适的值。
	
output_depth
	指定输出视频位深。
	
	仅当 precision_mode = 1 / 2 / 3 时可用。 
	
	范围：8 ~ 16
	默认值：自动选择最合适的值。
	
enable_fast_skip_plane
	启用后，当Y/Cb/Cr为0时，对应的平面会直接被复制到输出，忽略ditherY/ditherC的值。
	
	默认值：true

random_algo_ref / random_algo_dither
	选择参考像素位置/dither噪声的随机数生成算法。
	
	0：旧版滤镜算法
	1：均匀分布
	2：高斯分布 （标准差1.0，只使用[-1.0, 1.0]范围内的值）
	
	默认值：1 / 1

--------------------------------------------------------------------------------

f3kdb_dither(clip c, int "mode", bool "stacked", int "input_depth", 
             bool "keep_tv_range")

使用flash3kyuu_deband的dither算法把高位深视频转换为正常的8bit视频。

支持的颜色空间与flash3kyuu_deband相同。

需求支持SSE2的CPU (Pentium 4 / AMD K8 或更新)

mode
	0：Ordered dithering
	1：Floyd-Steinberg dithering
	
	默认值：1
	
stacked
	true：源数据为层叠格式 
		  (即 flash3kyuu_deband precision_mode = 4 的输出格式)
	false：源数据为交织格式 
		  (即 flash3kyuu_deband precision_mode = 5 的输出格式)
		  
	默认值：true
	
input_depth
	指定输入数据位深
	
	默认值：16
	有效范围：9 ~ 16
	
keep_tv_range
	请参考flash3kyuu_deband的同名参数
	
	默认值：false