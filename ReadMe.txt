flash3kyuu_deband(clip c, int "range", int "Y", int "Cb", int "Cr", 
		int "ditherY", int "ditherC", int "sample_mode", int "seed", 
		bool "blur_first", bool "diff_seed", int "opt", bool "mt", 
		int "precision_mode", bool "keep_tv_range")
		
Ported from http://www.geocities.jp/flash3kyuu/auf/banding17.zip . 
(I'm not the author of the original aviutl plugin, just ported the algorithm to
avisynth.)

This avisynth plugin debands the video by replacing banded pixels with average 
value of referenced pixels, and optionally dithers them.

Supported colorspaces: YUY2, YV12, YV16, YV24, YV411 (Progressive only)

Parameters:		

range
	Banding detection range. 
	
	Default: 15

Y Cb Cr
	Banding detection threshold. If difference between current pixel and 
	reference pixel is less than threshold, it will be considered as banded.
	
	If set to 0, the corresponding plane will be untouched regardless of dither
	settings.
	
	Default: 1 (precision_mode = 0) /
			 64 (precision_mode > 0)
	
ditherY ditherC
	Valid only when sample_mode is 1 or 2. Specifies dither strength.
	
	Default: 1 (precision_mode = 0) /
	         64 (precision_mode > 0)
	
sample_mode
	Valid modes are:
	
	0: Take 1 pixel as refernce pixel. Don't dither.
	1: Take 2 pixels as reference pixel. Reference pixels are in the same 
	   column of current pixel. Dither after processing.
	2: Take 4 pixels as reference pixel. Reference pixels are in the square 
	   around current pixel. Dither after processing.

	Default: 2
	
seed
	Seed for random number generation. 
	
blur_first
	Valid only when sample_mode is 1 or 2. 
	
	true: Current pixel is compared with average value of all pixels. 
	false: Current pixel is compared with all pixels. The pixel is considered as
	       banded pixel only if all differences are less than threshold.
	       	
	Default: true
	
diff_seed
	Use different seed for each frame.
	
	Caveats: 
	1. Speed may be significantly slower if enabled.
	2. The filter will become non-thread-safe if enabled. Avisynth MT mode 1 
	   will work incorrectly or even crash. 
	   Mode 2 should work though, of course appropriate overlap need to be set. 
	   (not tested)
	
	Default: false
	
opt
	Specifies optimization mode. 
	
	-1: Use highest optimization mode that is supported by host CPU
	0: No optimization (should be supported by almost all CPUs)
	1: SSE2 (Pentium 4, AMD K8)
	2: SSSE3 (Core 2)
	3: SSE4.1 (Core 2 45nm)
	
	Default: -1
	
mt
	Multi-threaded processing. If set to true, U and V plane will be proccessed 
	in parallel with Y plane to speed up processing.
	
	Like diff_seed, not compatible with Avisynth MT mode 1.
	
	Default: true if host has more than 1 CPU/cores, false otherwise.
	
precision_mode
	0: Low precision
	1: High precision, No dithering
	2: High precision, Ordered dithering
	3: High precision, Floyd-Steinberg dithering
	4: High precision, 16bit stacked output, for use with dither toolset
	   (http://forum.doom9.org/showthread.php?p=1386559#post1386559)
	5: High precision, 16bit interleaved output, for 10bit x264 encoding
	
	Note: 
	1. In sample mode 0, only mode 0 is available (it doesn't make sense to use
	   high precision mode). Setting this to other values will cause an error.
	
	2. It is not recommended to use mode 0 anymore (except that you want to use
	   sample mode 0). Use mode 1/2 for speed, or mode 3 for quality.
	   
	3. In high precision mode, threshold and dither parameters are 64 times
	   bigger than equivalent in low precision mode.
	   	      
	4. 10bit x264 command-line example:
	   avs2yuv -raw "script.avs" -o - | x264-10bit --demuxer raw --input-depth 16 --input-res 1280x720 --fps 24 --output "out.mp4" -
	   
	   Or compile x264 with the patch on https://gist.github.com/1117711, and 
	   specify the script directly:
	   x264-10bit --input-depth 16 --output "out.mp4" script.avs
	
	Default: 0 (sample_mode = 0) /
	         3 (sample_mode > 0)
	   
keep_tv_range
	If set to true, all processed pixels will be clamped to TV range 
	(luma: 16 ~ 235, chroma: 16 ~ 240). 
	
	It is recommended to set this to true for TV-range videos, since pixel 
	values may overflow/underflow after dithering. 
	
	DON'T set this to true for full-range videos, as all out-of-range pixels
	will be clamped to TV range.
	
	Default: false
	
--------------------------------------------------------------------------------

f3kdb_dither(clip c, int "mode", bool "stacked", int "input_depth", 
             bool "keep_tv_range")

Downsamples high bit-depth video to regular 8-bit video using dither routines 
of flash3kyuu_deband.

Supported colorspaces: The same as flash3kyuu_deband.

Requires SSE2-capable CPUs (Pentium 4 / AMD K8 or later)

Parameters:

mode
	0: Ordered dithering
	1: Floyd-Steinberg dithering
	
	Default: 1
	
stacked
	true: Input data is stacked 
		  (like output of flash3kyuu_deband precision_mode = 4)
	false: Input data is interleaved 
		  (like output of flash3kyuu_deband precision_mode = 5)
		  
	Default: true
	
input_depth
	Specifies input bit depth
	
	Default: 16
	Valid range: 9 ~ 16
	
keep_tv_range
	See description of keep_tv_range in flash3kyuu_deband
	
	Default: false