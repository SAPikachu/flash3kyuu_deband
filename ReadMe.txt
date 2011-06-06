flash3kyuu_deband(clip c, int "range", int "Y", int "Cb", int "Cr", 
		int "ditherY", int "ditherC", int "sample_mode", int "seed", 
		bool "blur_first", bool "diff_seed", int "opt", bool "mt", 
		int "precision_mode")
		
Ported from http://www.geocities.jp/flash3kyuu/auf/banding17.zip . 
(I'm not the author of the original aviutl plugin, just ported the algorithm to
avisynth.)

This avisynth plugin debands the video by replacing banded pixels with average 
value of referenced pixels, and optionally dithers them.

Only YV12 progressive sources are supported.

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
	   Mode 2 should work though, of course appropriate overlap need to be set. 
	   (not tested)
	
	Default: false
	
opt
	Specifies optimization mode. 
	(Currently only effective when precision_mode = 0)
	
	-1: Use highest optimization mode that is supported by host CPU
	0: No optimization (plain C, all CPU should be supported)
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
	
	Note: In high precision mode, threshold and dither parameters are 64 times
	      bigger than equivalent in low precision mode.
	
	Default: 3