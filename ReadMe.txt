flash3kyuu_deband(clip c, int "range", int "Y", int "Cb", int "Cr", 
		int "ditherY", int "ditherC", int "sample_mode", int "seed", 
		int "blur_first", int "diff_seed", int "opt")
		
Ported from http://www.geocities.jp/flash3kyuu/auf/banding17.zip . 
(I'm not the author of the original aviutl plugin, just ported the algorithm to
avisynth.)

This avisynth plugin debands the video by replacing banded pixels with average 
value of referenced pixels, and optionally dithers them.

Currently, all calculation are done in 8-bit. 12-bit dithering may be 
implemented in future.

Only YV12 progressive sources are supported.

Parameters:		

range
	Banding detection range. Note that pixels within all edges of frame are 
	treated as non-banded. Edge width equals to range.
	
	Default: 15

Y Cb Cr
	Banding detection threshold. If difference between current pixel and 
	reference pixel is less than threshold, it will be considered as banded.
	
	Default: 1
	
ditherY ditherC
	Valid only when sample_mode is 1 or 2. Specifies dither strength.
	
	Default: 1
	
sample_mode
	Valid modes are:
	
	0: Take 1 pixel as refernce pixel. Don't dither.
	1: Take 2 pixels as reference pixel. Reference pixels are in the same 
	   column of current pixel. Dither after processing.
	2: Take 4 pixels as reference pixel. Reference pixels are in the square 
	   around current pixel. Dither after processing.

	Default: 1
	
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
	   Mode 2 should work though (not tested).
	
	Default: false
	
opt
	Specifies optimization mode.
	
	-1: Use highest optimization mode that is supported by host CPU
	0: No optimization (plain C, all CPU should be supported)
	1: SSE2 (Pentium 4, AMD K8)
	2: SSSE3 (Core 2)
	3: SSE4.1 (Core 2 45nm)
	
	Default: -1