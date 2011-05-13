// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the FLASH3KYUU_DEBAND_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// FLASH3KYUU_DEBAND_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.

#include "avisynth.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <memory.h>
#include <malloc.h>

#include <smmintrin.h>

#ifdef FLASH3KYUU_DEBAND_EXPORTS
#define FLASH3KYUU_DEBAND_API extern "C" __declspec(dllexport)
#else
#define FLASH3KYUU_DEBAND_API extern "C" __declspec(dllimport)
#endif

AVSValue __cdecl Create_flash3kyuu_deband(AVSValue args, void* user_data, IScriptEnvironment* env);

FLASH3KYUU_DEBAND_API const char* __stdcall AvisynthPluginInit2(IScriptEnvironment* env);


typedef __declspec(align(4)) struct _pixel_dither_info {
	signed char ref1, ref2, change;
	signed char unused;
} pixel_dither_info;

// align lut for SSE operations
#define FRAME_LUT_ALIGNMENT 16

// whole multiples of alignment, so SSE codes don't need to check boundaries
#define FRAME_LUT_STRIDE(width) (((width - 1) | (FRAME_LUT_ALIGNMENT - 1)) + 1)

class flash3kyuu_deband : public GenericVideoFilter {
private:
	int _range_raw; 
	int _range; 
	int _Y, _Cb, _Cr;
	int _ditherY, _ditherC;

	int _sample_mode;
	int _seed;

	bool _blur_first;
	bool _diff_seed_for_each_frame;

	
	int _range_lut[64];
	int _ditherY_lut[64];
	int _ditherC_lut[64];

	signed char *_h_ind_masks;
	
	pixel_dither_info *_y_info;
	pixel_dither_info *_cb_info;
	pixel_dither_info *_cr_info;

	void init(void);
	void init_frame_luts(int n);
	void destroy_frame_luts(void);
	
	void process_plane(int n, PVideoFrame src, PVideoFrame dst, unsigned char *dstp, int plane);
	void process_plane_plainc(unsigned char const*srcp, int const src_width, int const src_height, int const src_pitch, unsigned char *dstp, int dst_pitch, int threshold, pixel_dither_info *info_ptr_base, int info_stride, int range);

public:
	flash3kyuu_deband(PClip child, int range, int Y, int Cb, int Cr, 
		int ditherY, int ditherC, int sample_mode, int seed,
		bool blur_first, bool diff_seed_for_each_frame);
	~flash3kyuu_deband();

	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};