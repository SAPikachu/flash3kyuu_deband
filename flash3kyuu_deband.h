// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the FLASH3KYUU_DEBAND_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// FLASH3KYUU_DEBAND_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.

#include "avisynth.h"
#include <string.h>
#include <stdio.h>

#ifdef FLASH3KYUU_DEBAND_EXPORTS
#define FLASH3KYUU_DEBAND_API extern "C" __declspec(dllexport)
#else
#define FLASH3KYUU_DEBAND_API extern "C" __declspec(dllimport)
#endif

AVSValue __cdecl Create_flash3kyuu_deband(AVSValue args, void* user_data, IScriptEnvironment* env);

FLASH3KYUU_DEBAND_API const char* __stdcall AvisynthPluginInit2(IScriptEnvironment* env);

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

	void init(void);

	void process_pixel_out_of_range_mode0(const unsigned char* src_px, int src_pitch, unsigned char* dst_px, int seed, int* dither_lut, int shift_bits, int ind_mask, int threshold, bool is_full_plane);
	void process_pixel_out_of_range_mode12(const unsigned char* src_px, int src_pitch, unsigned char* dst_px, int seed, int* dither_lut, int shift_bits, int ind_mask, int threshold, bool is_full_plane);
	
	void process_pixel_mode0(const unsigned char* src_px, int src_pitch, unsigned char* dst_px, int seed, int* dither_lut, int shift_bits, int ind_mask, int threshold, bool is_full_plane);
	
	void process_pixel_mode1_noblur(const unsigned char* src_px, int src_pitch, unsigned char* dst_px, int seed, int* dither_lut, int shift_bits, int ind_mask, int threshold, bool is_full_plane);
	void process_pixel_mode1_blur(const unsigned char* src_px, int src_pitch, unsigned char* dst_px, int seed, int* dither_lut, int shift_bits, int ind_mask, int threshold, bool is_full_plane);

	void process_pixel_mode2_noblur(const unsigned char* src_px, int src_pitch, unsigned char* dst_px, int seed, int* dither_lut, int shift_bits, int ind_mask, int threshold, bool is_full_plane);
	void process_pixel_mode2_blur(const unsigned char* src_px, int src_pitch, unsigned char* dst_px, int seed, int* dither_lut, int shift_bits, int ind_mask, int threshold, bool is_full_plane);

	void process_plane(int n, PVideoFrame src, PVideoFrame dst, unsigned char *dstp, int plane);
public:
	flash3kyuu_deband(PClip child, int range, int Y, int Cb, int Cr, 
		int ditherY, int ditherC, int sample_mode, int seed,
		bool blur_first, bool diff_seed_for_each_frame);
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};